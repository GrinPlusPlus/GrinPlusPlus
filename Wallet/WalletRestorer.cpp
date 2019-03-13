#include "WalletRestorer.h"
#include "Keychain/KeyChain.h"
#include "WalletUtil.h"

#include <Consensus/BlockTime.h>

static const uint64_t NUM_OUTPUTS_PER_BATCH = 1000;

WalletRestorer::WalletRestorer(const Config& config, const INodeClient& nodeClient, const KeyChain& keyChain)
	: m_config(config), m_nodeClient(nodeClient), m_keyChain(keyChain)
{

}

bool WalletRestorer::Restore(const CBigInteger<32>& masterSeed, Wallet& wallet) const
{
	const uint64_t chainHeight = m_nodeClient.GetChainHeight();

	std::vector<OutputData> walletOutputs;
	uint64_t nextLeafIndex = 0;
	while (true)
	{
		std::unique_ptr<OutputRange> pOutputRange = m_nodeClient.GetOutputsByLeafIndex(nextLeafIndex, NUM_OUTPUTS_PER_BATCH);
		if (pOutputRange == nullptr)
		{
			return false;
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

	if (walletOutputs.empty())
	{
		return true;
	}

	// TODO: Restore nextChildIndices
	// TODO: Create WalletTxs

	return wallet.AddOutputs(masterSeed, walletOutputs);
}

std::unique_ptr<OutputData> WalletRestorer::GetWalletOutput(const CBigInteger<32>& masterSeed, const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const
{
	std::unique_ptr<RewoundProof> pRewoundProof = m_keyChain.RewindRangeProof(outputDisplayInfo.GetIdentifier().GetCommitment(), outputDisplayInfo.GetRangeProof());
	if (pRewoundProof != nullptr)
	{
		KeyChainPath keyChainPath(pRewoundProof->GetProofMessage().ToKeyIndices(3)); // TODO: Always length 3 for now. Need to grind through in future.
		TransactionOutput txOutput(outputDisplayInfo.GetIdentifier().GetFeatures(), Commitment(outputDisplayInfo.GetIdentifier().GetCommitment()), RangeProof(outputDisplayInfo.GetRangeProof()));
		const uint64_t amount = pRewoundProof->GetAmount();
		const EOutputStatus status = DetermineStatus(outputDisplayInfo, currentBlockHeight);
		const uint64_t mmrIndex = outputDisplayInfo.GetLocation().GetMMRIndex();

		return std::make_unique<OutputData>(OutputData(std::move(keyChainPath), std::move(txOutput), amount, status, std::make_optional<uint64_t>(mmrIndex)));
	}

	return std::unique_ptr<OutputData>(nullptr);
}

EOutputStatus WalletRestorer::DetermineStatus(const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const
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