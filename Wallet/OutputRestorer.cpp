#include "OutputRestorer.h"
#include "Keychain/KeyChain.h"

#include <Wallet/WalletUtil.h>
#include <Consensus/BlockTime.h>
#include <Consensus/HardForks.h>

static const uint64_t NUM_OUTPUTS_PER_BATCH = 1000;

OutputRestorer::OutputRestorer(const Config& config, const INodeClient& nodeClient, const KeyChain& keyChain)
	: m_config(config), m_nodeClient(nodeClient), m_keyChain(keyChain)
{

}

std::vector<OutputData> OutputRestorer::FindAndRewindOutputs(const SecureVector& masterSeed, Wallet& wallet, const bool fromGenesis) const
{
	const uint64_t chainHeight = m_nodeClient.GetChainHeight();

	uint64_t nextLeafIndex = fromGenesis ? 0 : wallet.GetRestoreLeafIndex() + 1;

	std::vector<OutputData> walletOutputs;
	while (true)
	{
		std::unique_ptr<OutputRange> pOutputRange = m_nodeClient.GetOutputsByLeafIndex(nextLeafIndex, NUM_OUTPUTS_PER_BATCH);
		if (pOutputRange == nullptr)
		{
			return std::vector<OutputData>();
		}

		// No new outputs since last restore
		if (pOutputRange->GetLastRetrievedIndex() == 0)
		{
			return std::vector<OutputData>();
		}

		const std::vector<OutputDisplayInfo>& outputs = pOutputRange->GetOutputs();
		for (const OutputDisplayInfo& output : outputs)
		{
			std::unique_ptr<OutputData> pOutputData = GetWalletOutput(masterSeed, output, chainHeight);
			if (pOutputData != nullptr)
			{
				walletOutputs.emplace_back(*pOutputData);
			}
		}

		nextLeafIndex = pOutputRange->GetLastRetrievedIndex() + 1;
		if (nextLeafIndex > pOutputRange->GetHighestIndex())
		{
			break;
		}
	}

	wallet.SetRestoreLeafIndex(nextLeafIndex - 1);

	return walletOutputs;
}

std::unique_ptr<OutputData> OutputRestorer::GetWalletOutput(const SecureVector& masterSeed, const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const
{
	EBulletproofType type = EBulletproofType::ORIGINAL;
	std::unique_ptr<RewoundProof> pRewoundProof = nullptr;
	if (Consensus::GetHeaderVersion(m_config.GetEnvironment().GetEnvironmentType(), ((std::max)(currentBlockHeight, 2 * Consensus::WEEK_HEIGHT) - (2 * Consensus::WEEK_HEIGHT))) == 1)
	{
		pRewoundProof = m_keyChain.RewindRangeProof(outputDisplayInfo.GetIdentifier().GetCommitment(), outputDisplayInfo.GetRangeProof(), EBulletproofType::ORIGINAL);
	}

	if (pRewoundProof == nullptr)
	{
		type = EBulletproofType::ENHANCED;
		pRewoundProof = m_keyChain.RewindRangeProof(outputDisplayInfo.GetIdentifier().GetCommitment(), outputDisplayInfo.GetRangeProof(), EBulletproofType::ENHANCED);
	}

	if (pRewoundProof != nullptr)
	{
		KeyChainPath keyChainPath(pRewoundProof->GetProofMessage().ToKeyIndices(type));
		const std::unique_ptr<SecretKey>& pBlindingFactor = pRewoundProof->GetBlindingFactor();
		CBigInteger<32> blindingFactor;
		if (pBlindingFactor != nullptr && type == EBulletproofType::ORIGINAL)
		{
			blindingFactor = pBlindingFactor->GetBytes();
		}
		else
		{
			blindingFactor = m_keyChain.DerivePrivateKey(keyChainPath, pRewoundProof->GetAmount())->GetBytes();
		}

		TransactionOutput txOutput(outputDisplayInfo.GetIdentifier().GetFeatures(), Commitment(outputDisplayInfo.GetIdentifier().GetCommitment()), RangeProof(outputDisplayInfo.GetRangeProof()));
		const uint64_t amount = pRewoundProof->GetAmount();
		const EOutputStatus status = DetermineStatus(outputDisplayInfo, currentBlockHeight);
		const uint64_t mmrIndex = outputDisplayInfo.GetLocation().GetMMRIndex();
		const uint64_t blockHeight = outputDisplayInfo.GetLocation().GetBlockHeight();

		return std::make_unique<OutputData>(
			std::move(keyChainPath), 
			SecretKey(std::move(blindingFactor)), 
			std::move(txOutput), 
			amount, 
			status, 
			std::make_optional<uint64_t>(mmrIndex), 
			std::make_optional<uint64_t>(blockHeight),
			std::nullopt
		);
	}

	return std::unique_ptr<OutputData>(nullptr);
}

EOutputStatus OutputRestorer::DetermineStatus(const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const
{
	if (outputDisplayInfo.IsSpent())
	{
		return EOutputStatus::SPENT;
	}

	const EOutputFeatures features = outputDisplayInfo.GetIdentifier().GetFeatures();
	const uint64_t outputBlockHeight = outputDisplayInfo.GetLocation().GetBlockHeight();
	const uint32_t minimumConfirmations = m_config.GetWalletConfig().GetMinimumConfirmations();

	if (WalletUtil::IsOutputImmature(features, outputBlockHeight, currentBlockHeight, minimumConfirmations))
	{
		return EOutputStatus::IMMATURE;
	}

	return EOutputStatus::SPENDABLE;
}