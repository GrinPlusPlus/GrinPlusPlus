#include "WalletRestorer.h"
#include "Keychain/KeyChain.h"

#include <Wallet/WalletUtil.h>
#include <Consensus/BlockTime.h>

static const uint64_t NUM_OUTPUTS_PER_BATCH = 1000;

WalletRestorer::WalletRestorer(const Config& config, const INodeClient& nodeClient, const KeyChain& keyChain)
	: m_config(config), m_nodeClient(nodeClient), m_keyChain(keyChain)
{

}

bool WalletRestorer::Restore(const SecureVector& masterSeed, Wallet& wallet, const bool fromGenesis) const
{
	const uint64_t chainHeight = m_nodeClient.GetChainHeight();

	uint64_t nextLeafIndex = fromGenesis ? 0 : wallet.GetRestoreLeafIndex() + 1;

	std::vector<OutputData> walletOutputs;
	while (true)
	{

		//const std::vector<BlockWithOutputs> blocksWithOutputs = m_nodeClient.GetBlockOutputs(nextHeight, restoreHeight);
		//if (blocksWithOutputs.empty())
		//{
		//	return false;
		//}

		//for (const BlockWithOutputs& block : blocksWithOutputs)
		//{
		//	for (const OutputDisplayInfo& output : block.GetOutputs())
		//	{
		//		std::unique_ptr<OutputData> pOutputData = GetWalletOutput(masterSeed, output, restoreHeight);
		//		if (pOutputData != nullptr)
		//		{
		//			walletOutputs.emplace_back(*pOutputData);
		//		}
		//	}

		//	nextHeight = block.GetBlockIdentifier().GetHeight() + 1;
		//}

		//if (nextHeight > restoreHeight)
		//{
		//	break;
		//}

		std::unique_ptr<OutputRange> pOutputRange = m_nodeClient.GetOutputsByLeafIndex(nextLeafIndex, NUM_OUTPUTS_PER_BATCH);
		if (pOutputRange == nullptr)
		{
			return false;
		}

		// No new outputs since last restore
		if (pOutputRange->GetLastRetrievedIndex() == 0)
		{
			return true;
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
		return wallet.SetRestoreLeafIndex(nextLeafIndex - 1);
	}

	return SaveWalletOutputs(masterSeed, wallet, walletOutputs, nextLeafIndex - 1);
}

std::unique_ptr<OutputData> WalletRestorer::GetWalletOutput(const SecureVector& masterSeed, const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const
{
	std::unique_ptr<RewoundProof> pRewoundProof = m_keyChain.RewindRangeProof(outputDisplayInfo.GetIdentifier().GetCommitment(), outputDisplayInfo.GetRangeProof());
	if (pRewoundProof != nullptr)
	{
		KeyChainPath keyChainPath(pRewoundProof->GetProofMessage().ToKeyIndices(3)); // TODO: Always length 3 for now. Need to grind through in future.
		SecretKey blindingFactor(pRewoundProof->GetBlindingFactor());
		TransactionOutput txOutput(outputDisplayInfo.GetIdentifier().GetFeatures(), Commitment(outputDisplayInfo.GetIdentifier().GetCommitment()), RangeProof(outputDisplayInfo.GetRangeProof()));
		const uint64_t amount = pRewoundProof->GetAmount();
		const EOutputStatus status = DetermineStatus(outputDisplayInfo, currentBlockHeight);
		const uint64_t mmrIndex = outputDisplayInfo.GetLocation().GetMMRIndex();
		const uint64_t blockHeight = outputDisplayInfo.GetLocation().GetBlockHeight();

		return std::make_unique<OutputData>(std::move(keyChainPath), std::move(blindingFactor), std::move(txOutput), amount, status, std::make_optional<uint64_t>(mmrIndex), std::make_optional<uint64_t>(blockHeight));
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

bool WalletRestorer::SaveWalletOutputs(const SecureVector& masterSeed, Wallet& wallet, const std::vector<OutputData>& outputs, const uint64_t restoreLeafIndex) const
{
	// TODO: Restore nextChildIndices
	std::vector<OutputData> outputsToAdd;

	const std::vector<OutputData> savedOutputs = wallet.RefreshOutputs(masterSeed);
	for (const OutputData& output : outputs)
	{
		if (IsNewOutput(masterSeed, wallet, output, savedOutputs))
		{
			outputsToAdd.push_back(output);
		}
	}

	bool added = wallet.AddRestoredOutputs(masterSeed, outputsToAdd);
	if (!added)
	{
		return false;
	}

	return wallet.SetRestoreLeafIndex(restoreLeafIndex);
}

bool WalletRestorer::IsNewOutput(const SecureVector& masterSeed, Wallet& wallet, const OutputData& output, const std::vector<OutputData>& existingOutputs) const
{
	for (const OutputData& existingOutput : existingOutputs)
	{
		if (existingOutput.GetKeyChainPath() == output.GetKeyChainPath())
		{
			if (existingOutput.GetMMRIndex().has_value())
			{
				if (existingOutput.GetMMRIndex().value() == output.GetMMRIndex().value())
				{
					return false;
				}

				continue;
			}
			else if (existingOutput.GetStatus() == EOutputStatus::SPENT)
			{
				continue;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}