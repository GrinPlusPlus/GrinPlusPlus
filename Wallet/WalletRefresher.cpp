#include "WalletRefresher.h"

#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>

WalletRefresher::WalletRefresher(const INodeClient& nodeClient, IWalletDB& walletDB)
	: m_nodeClient(nodeClient), m_walletDB(walletDB)
{

}

std::vector<OutputData> WalletRefresher::RefreshOutputs(const std::string& username, const CBigInteger<32>& masterSeed, const uint64_t minimumConfirmations)
{
	std::vector<Commitment> commitments;

	std::vector<OutputData> outputs = m_walletDB.GetOutputs(username, masterSeed);
	for (const OutputData& outputData : outputs)
	{
		if (outputData.GetStatus() != EOutputStatus::SPENT)
		{
			// TODO: What if commitment has mmr_index?
			commitments.push_back(outputData.GetOutput().GetCommitment());
		}
	}

	const uint64_t lastConfirmedHeight = m_nodeClient.GetChainHeight();
	// TODO: Update lastRefreshedHeight in DB

	std::vector<OutputData> outputsToUpdate;
	const std::map<Commitment, OutputLocation> outputLocations = m_nodeClient.GetOutputsByCommitment(commitments);
	for (OutputData& outputData : outputs)
	{
		// TODO: Figure out how to tell the difference between spent and awaitingConfirmation
		auto iter = outputLocations.find(outputData.GetOutput().GetCommitment());
		if (iter != outputLocations.cend())
		{
			if (outputData.GetStatus() != EOutputStatus::LOCKED)
			{
				const OutputLocation& outputLocation = iter->second;
				if ((outputLocation.GetBlockHeight() + minimumConfirmations) <= lastConfirmedHeight)
				{
					if (outputData.GetStatus() != EOutputStatus::SPENDABLE)
					{
						outputData.SetStatus(EOutputStatus::SPENDABLE);
						outputsToUpdate.push_back(outputData);
					}
				}
				else
				{
					if (outputData.GetStatus() != EOutputStatus::IMMATURE)
					{
						outputData.SetStatus(EOutputStatus::IMMATURE);
						outputsToUpdate.push_back(outputData);
					}
				}
			}
		}
		else if (outputData.GetStatus() != EOutputStatus::NO_CONFIRMATIONS)
		{
			outputData.SetStatus(EOutputStatus::SPENT);
			outputsToUpdate.push_back(outputData);
		}
	}

	m_walletDB.AddOutputs(username, masterSeed, outputsToUpdate);

	return outputs;
}