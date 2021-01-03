#include "OutputRestorer.h"

#include <Consensus.h>
#include <Wallet/Keychain/KeyChain.h>
#include <Wallet/WalletUtil.h>
#include <Common/Logger.h>

static const uint64_t NUM_OUTPUTS_PER_BATCH = 1000;

std::vector<OutputDataEntity> OutputRestorer::FindAndRewindOutputs(const std::shared_ptr<IWalletDB>& pBatch, const bool fromGenesis) const
{
	const uint64_t chainHeight = m_pNodeClient->GetChainHeight();

	uint64_t nextLeafIndex = fromGenesis ? 0 : pBatch->GetRestoreLeafIndex() + 1;
	uint64_t highestIndex = 0;

	std::vector<OutputDataEntity> walletOutputs;
	while (true)
	{
		std::unique_ptr<OutputRange> pOutputRange = m_pNodeClient->GetOutputsByLeafIndex(nextLeafIndex, NUM_OUTPUTS_PER_BATCH);
		if (pOutputRange == nullptr || pOutputRange->GetLastRetrievedIndex() == 0) {
			// No new outputs since last restore
			return std::vector<OutputDataEntity>();
		}

		const std::vector<OutputDTO>& outputs = pOutputRange->GetOutputs();
		for (const OutputDTO& output : outputs)
		{
			std::unique_ptr<OutputDataEntity> pOutputDataEntity = GetWalletOutput(output, chainHeight);
			if (pOutputDataEntity != nullptr) {
				walletOutputs.emplace_back(*pOutputDataEntity);
			}
		}

		if (highestIndex == 0) {
			// Cache this, rather than use the new response from pOutputRange.
			// Otherwise, pOutputRange->GetHighestIndex() could continue to rise slowly during sync, tying up this thread.
			highestIndex = pOutputRange->GetHighestIndex();
		}

		nextLeafIndex = pOutputRange->GetLastRetrievedIndex() + 1;
		if (nextLeafIndex > highestIndex) {
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
	if (Consensus::GetHeaderVersion(((std::max)(outputBlockHeight, 2 * Consensus::WEEK_HEIGHT) - (2 * Consensus::WEEK_HEIGHT))) == 1)
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
	const uint32_t minimumConfirmations = m_config.GetMinimumConfirmations();

	if (WalletUtil::IsOutputImmature(features, outputBlockHeight, currentBlockHeight, minimumConfirmations))
	{
		return EOutputStatus::IMMATURE;
	}

	return EOutputStatus::SPENDABLE;
}