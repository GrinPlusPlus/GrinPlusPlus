#include "OutputRestorer.h"
#include <Wallet/Keychain/KeyChain.h>

#include <Wallet/WalletUtil.h>
#include <Consensus/BlockTime.h>
#include <Consensus/HardForks.h>
#include <Infrastructure/Logger.h>

static const uint64_t NUM_OUTPUTS_PER_BATCH = 1000;

OutputRestorer::OutputRestorer(const Config& config, INodeClientConstPtr pNodeClient, const KeyChain& keyChain)
	: m_config(config), m_pNodeClient(pNodeClient), m_keyChain(keyChain)
{

}

std::vector<OutputDataEntity> OutputRestorer::FindAndRewindOutputs(Writer<IWalletDB> pBatch, const bool fromGenesis) const
{
	const uint64_t chainHeight = m_pNodeClient->GetChainHeight();

	uint64_t nextLeafIndex = fromGenesis ? 0 : pBatch->GetRestoreLeafIndex() + 1;

	std::vector<OutputDataEntity> walletOutputs;
	while (true)
	{
		std::unique_ptr<OutputRange> pOutputRange = m_pNodeClient->GetOutputsByLeafIndex(nextLeafIndex, NUM_OUTPUTS_PER_BATCH);
		if (pOutputRange == nullptr || pOutputRange->GetLastRetrievedIndex() == 0)
		{
			// No new outputs since last restore
			return std::vector<OutputDataEntity>();
		}

		const std::vector<OutputDTO>& outputs = pOutputRange->GetOutputs();
		for (const OutputDTO& output : outputs)
		{
			std::unique_ptr<OutputDataEntity> pOutputDataEntity = GetWalletOutput(output, chainHeight);
			if (pOutputDataEntity != nullptr)
			{
				walletOutputs.emplace_back(*pOutputDataEntity);
			}
		}

		nextLeafIndex = pOutputRange->GetLastRetrievedIndex() + 1;
		if (nextLeafIndex > pOutputRange->GetHighestIndex())
		{
			break;
		}
	}

	pBatch->UpdateRestoreLeafIndex(nextLeafIndex - 1);

	return walletOutputs;
}

std::unique_ptr<OutputDataEntity> OutputRestorer::GetWalletOutput(const OutputDTO& output, const uint64_t currentBlockHeight) const
{
	EBulletproofType type = EBulletproofType::ORIGINAL;
	std::unique_ptr<RewoundProof> pRewoundProof = nullptr;
	const uint64_t outputBlockHeight = output.GetLocation().GetBlockHeight();
	if (Consensus::GetHeaderVersion(m_config.GetEnvironment().GetEnvironmentType(), ((std::max)(outputBlockHeight, 2 * Consensus::WEEK_HEIGHT) - (2 * Consensus::WEEK_HEIGHT))) == 1)
	{
		pRewoundProof = m_keyChain.RewindRangeProof(output.GetIdentifier().GetCommitment(), output.GetRangeProof(), EBulletproofType::ORIGINAL);
	}

	if (pRewoundProof == nullptr)
	{
		type = EBulletproofType::ENHANCED;
		pRewoundProof = m_keyChain.RewindRangeProof(output.GetIdentifier().GetCommitment(), output.GetRangeProof(), EBulletproofType::ENHANCED);
	}

	if (pRewoundProof != nullptr)
	{
		WALLET_INFO_F("Found own output: {}", output.GetIdentifier().GetCommitment());

		KeyChainPath keyChainPath(pRewoundProof->GetProofMessage().ToKeyIndices(type));
		const std::unique_ptr<SecretKey>& pBlindingFactor = pRewoundProof->GetBlindingFactor();
		BlindingFactor blindingFactor(ZERO_HASH);
		if (pBlindingFactor != nullptr && type == EBulletproofType::ORIGINAL)
		{
			blindingFactor = pBlindingFactor->GetBytes();
		}
		else
		{
			blindingFactor = m_keyChain.DerivePrivateKey(keyChainPath, pRewoundProof->GetAmount()).GetBytes();
		}

		TransactionOutput txOutput(
			output.GetIdentifier().GetFeatures(),
			Commitment(output.GetIdentifier().GetCommitment()),
			RangeProof(output.GetRangeProof())
		);
		const uint64_t amount = pRewoundProof->GetAmount();
		const EOutputStatus status = DetermineStatus(output, currentBlockHeight);
		const uint64_t mmrIndex = output.GetLocation().GetMMRIndex();
		const uint64_t blockHeight = output.GetLocation().GetBlockHeight();

		return std::make_unique<OutputDataEntity>(
			std::move(keyChainPath), 
			blindingFactor.ToSecretKey(), 
			std::move(txOutput), 
			amount, 
			status, 
			std::make_optional(mmrIndex), 
			std::make_optional(blockHeight),
			std::nullopt,
			std::nullopt
		);
	}

	return nullptr;
}

EOutputStatus OutputRestorer::DetermineStatus(const OutputDTO& output, const uint64_t currentBlockHeight) const
{
	if (output.IsSpent())
	{
		return EOutputStatus::SPENT;
	}

	const EOutputFeatures features = output.GetIdentifier().GetFeatures();
	const uint64_t outputBlockHeight = output.GetLocation().GetBlockHeight();
	const uint32_t minimumConfirmations = m_config.GetWalletConfig().GetMinimumConfirmations();

	if (WalletUtil::IsOutputImmature(m_config.GetEnvironment().GetEnvironmentType(), features, outputBlockHeight, currentBlockHeight, minimumConfirmations))
	{
		return EOutputStatus::IMMATURE;
	}

	return EOutputStatus::SPENDABLE;
}