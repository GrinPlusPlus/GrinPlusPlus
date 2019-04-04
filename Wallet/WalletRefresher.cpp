#include "WalletRefresher.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>

WalletRefresher::WalletRefresher(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB)
	: m_config(config), m_nodeClient(nodeClient), m_walletDB(walletDB)
{

}

std::vector<OutputData> WalletRefresher::RefreshOutputs(const std::string& username, const SecureVector& masterSeed)
{
	std::vector<Commitment> commitments;

	std::vector<OutputData> outputs = m_walletDB.GetOutputs(username, masterSeed);
	for (const OutputData& outputData : outputs)
	{
		//if (outputData.GetStatus() != EOutputStatus::SPENT && outputData.GetStatus() != EOutputStatus::CANCELED)
		{
			// TODO: What if commitment has mmr_index?
			commitments.push_back(outputData.GetOutput().GetCommitment());
		}
	}

	const uint64_t lastConfirmedHeight = m_nodeClient.GetChainHeight();
	m_walletDB.UpdateRefreshBlockHeight(username, lastConfirmedHeight);

	std::vector<OutputData> outputsToUpdate;
	const std::map<Commitment, OutputLocation> outputLocations = m_nodeClient.GetOutputsByCommitment(commitments);
	for (OutputData& outputData : outputs)
	{
		auto iter = outputLocations.find(outputData.GetOutput().GetCommitment());
		if (iter != outputLocations.cend())
		{
			if (outputData.GetStatus() != EOutputStatus::LOCKED)
			{
				const EOutputFeatures features = outputData.GetOutput().GetFeatures();
				const uint64_t outputBlockHeight = iter->second.GetBlockHeight();
				const uint32_t minimumConfirmations = m_config.GetWalletConfig().GetMinimumConfirmations();

				if (WalletUtil::IsOutputImmature(features, outputBlockHeight, lastConfirmedHeight, minimumConfirmations))
				{
					if (outputData.GetStatus() != EOutputStatus::IMMATURE)
					{
						outputData.SetBlockHeight(outputBlockHeight);
						outputData.SetStatus(EOutputStatus::IMMATURE);
						outputsToUpdate.push_back(outputData);
					}
				}
				else
				{
					if (outputData.GetStatus() != EOutputStatus::SPENDABLE)
					{
						outputData.SetBlockHeight(outputBlockHeight);
						outputData.SetStatus(EOutputStatus::SPENDABLE);
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
	RefreshTransactions(username, masterSeed, outputsToUpdate);

	return outputs;
}

void WalletRefresher::RefreshTransactions(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& refreshedOutputs)
{
	std::vector<WalletTx> transactions = m_walletDB.GetTransactions(username, masterSeed);
	for (WalletTx& walletTx : transactions)
	{
		if (walletTx.GetTransaction().has_value())
		{
			const std::vector<TransactionOutput>& outputs = walletTx.GetTransaction().value().GetBody().GetOutputs();
			for (const TransactionOutput& output : outputs)
			{
				std::unique_ptr<OutputData> pOutputData = FindOutput(refreshedOutputs, output.GetCommitment());
				if (pOutputData != nullptr)
				{
					if (pOutputData->GetBlockHeight().has_value())
					{
						walletTx.SetConfirmedHeight(pOutputData->GetBlockHeight().value());
					}

					if (walletTx.GetType() == EWalletTxType::SENDING_FINALIZED)
					{
						walletTx.SetType(EWalletTxType::SENT);
						m_walletDB.AddTransaction(username, masterSeed, walletTx);
					}
					else if (walletTx.GetType() == EWalletTxType::RECEIVING_IN_PROGRESS)
					{
						walletTx.SetType(EWalletTxType::RECEIVED);
						m_walletDB.AddTransaction(username, masterSeed, walletTx);
					}

					break;
				}
			}
		}
	}
}

std::unique_ptr<OutputData> WalletRefresher::FindOutput(const std::vector<OutputData>& refreshedOutputs, const Commitment& commitment) const
{
	for (const OutputData& outputData : refreshedOutputs)
	{
		if (commitment == outputData.GetOutput().GetCommitment())
		{
			return std::make_unique<OutputData>(OutputData(outputData));
		}
	}

	return std::unique_ptr<OutputData>(nullptr);
}