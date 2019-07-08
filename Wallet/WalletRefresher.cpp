#include "WalletRefresher.h"
#include "OutputRestorer.h"

#include <Infrastructure/Logger.h>
#include <Wallet/WalletUtil.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <unordered_map>

WalletRefresher::WalletRefresher(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB)
	: m_config(config), m_nodeClient(nodeClient), m_walletDB(walletDB)
{

}

// TODO: Rewrite this
// 0. Initial login after upgrade - for every output, find matching WalletTx and update OutputData TxId. If none found, create new WalletTx.
//
// 1. Check for own outputs in new blocks.
// 2. For each output, look for OutputData with matching commitment. If no output found, create new WalletTx and OutputData.
// 3. Refresh status for all OutputData by calling m_nodeClient.GetOutputsByCommitment
// 4. For all OutputData, update matching WalletTx status.

std::vector<OutputData> WalletRefresher::Refresh(const SecureVector& masterSeed, Wallet& wallet, const bool fromGenesis)
{
	LoggerAPI::LogInfo("WalletRefresher::Refresh - Refreshing wallet for user: " + wallet.GetUsername());

	if (m_nodeClient.GetChainHeight() < m_walletDB.GetRefreshBlockHeight(wallet.GetUsername()))
	{
		LoggerAPI::LogInfo("WalletRefresher::Refresh - Skipping refresh since node is resyncing.");
		return std::vector<OutputData>();
	}

	std::vector<OutputData> walletOutputs = m_walletDB.GetOutputs(wallet.GetUsername(), masterSeed);
	std::vector<WalletTx> walletTransactions = m_walletDB.GetTransactions(wallet.GetUsername(), masterSeed);

	// 1. Check for own outputs in new blocks.
	KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);
	std::vector<OutputData> restoredOutputs = OutputRestorer(m_config, m_nodeClient, keyChain).FindAndRewindOutputs(masterSeed, wallet, fromGenesis);

	// 2. For each restored output, look for OutputData with matching commitment.
	for (OutputData& restoredOutput : restoredOutputs)
	{
		LoggerAPI::LogInfo("WalletRefresher::Refresh - Output found at index " + std::to_string(restoredOutput.GetMMRIndex().value_or(0)));

		if (restoredOutput.GetStatus() != EOutputStatus::SPENT)
		{
			const Commitment& commitment = restoredOutput.GetOutput().GetCommitment();
			std::unique_ptr<OutputData> pExistingOutput = FindOutput(walletOutputs, commitment);
			if (pExistingOutput == nullptr)
			{
				LoggerAPI::LogInfo("WalletRefresher::Refresh - Restoring unknown output with commitment: " + commitment.ToHex());

				// If no output found, create new WalletTx and OutputData.
				const uint32_t walletTxId = wallet.GetNextWalletTxId();
				WalletTx walletTx(
					walletTxId,
					EWalletTxType::RECEIVED,
					std::nullopt,
					std::nullopt,
					std::chrono::system_clock::now(),
					std::nullopt, // TODO: Lookup block time
					restoredOutput.GetBlockHeight(),
					restoredOutput.GetAmount(),
					0,
					std::nullopt,
					std::nullopt
				);

				restoredOutput.SetWalletTxId(walletTxId);
				m_walletDB.AddOutputs(wallet.GetUsername(), masterSeed, std::vector<OutputData>({ restoredOutput }));
				m_walletDB.AddTransaction(wallet.GetUsername(), masterSeed, walletTx);

				walletOutputs.push_back(restoredOutput);
				walletTransactions.emplace_back(std::move(walletTx));
			}
			//else if (pExistingOutput->GetStatus() == EOutputStatus::SPENT || pExistingOutput->GetStatus() == EOutputStatus::CANCELED)
			//{

			//}
		}
	}

	// 3. Refresh status for all OutputData by calling m_nodeClient.GetOutputsByCommitment
	RefreshOutputs(masterSeed, wallet, walletOutputs);

	// 4. For all OutputData, update matching WalletTx status.
	RefreshTransactions(wallet.GetUsername(), masterSeed, walletOutputs, walletTransactions);

	return walletOutputs;
}

void WalletRefresher::RefreshOutputs(const SecureVector& masterSeed, Wallet& wallet, std::vector<OutputData>& walletOutputs)
{
	std::vector<Commitment> commitments;

	for (const OutputData& outputData : walletOutputs)
	{
		const Commitment& commitment = outputData.GetOutput().GetCommitment();

		/*if (outputData.GetStatus() == EOutputStatus::SPENT)// || outputData.GetStatus() == EOutputStatus::CANCELED)
		{
			LoggerAPI::LogTrace("WalletRefresher::RefreshOutputs - No need to refresh spent/canceled output with commitment: " + commitment.ToHex());
			continue;
		}*/

		// TODO: What if commitment has mmr_index?
		LoggerAPI::LogDebug("WalletRefresher::RefreshOutputs - Refreshing output with commitment: " + commitment.ToHex());
		commitments.push_back(commitment);		
	}

	const uint64_t lastConfirmedHeight = m_nodeClient.GetChainHeight();

	std::vector<OutputData> outputsToUpdate;
	const std::map<Commitment, OutputLocation> outputLocations = m_nodeClient.GetOutputsByCommitment(commitments);
	for (OutputData& outputData : walletOutputs)
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
						LoggerAPI::LogDebug("WalletRefresher::RefreshOutputs - Marking output as immature: " + outputData.GetOutput().GetCommitment().ToHex());

						outputData.SetBlockHeight(outputBlockHeight);
						outputData.SetStatus(EOutputStatus::IMMATURE);
						outputsToUpdate.push_back(outputData);
					}
				}
				else if (outputData.GetStatus() != EOutputStatus::SPENDABLE)
				{
					LoggerAPI::LogDebug("WalletRefresher::RefreshOutputs - Marking output as spendable: " + outputData.GetOutput().GetCommitment().ToHex());

					outputData.SetBlockHeight(outputBlockHeight);
					outputData.SetStatus(EOutputStatus::SPENDABLE);
					outputsToUpdate.push_back(outputData);
				}
			}
		}
		else if (
			outputData.GetStatus() != EOutputStatus::NO_CONFIRMATIONS && 
			outputData.GetStatus() != EOutputStatus::SPENT && 
			outputData.GetStatus() != EOutputStatus::CANCELED)
		{
			// TODO: Check if (lastConfirmedHeight > outputData.GetConfirmedHeight)
			LoggerAPI::LogDebug("WalletRefresher::RefreshOutputs - Marking output as spent: " + outputData.GetOutput().GetCommitment().ToHex());

			outputData.SetStatus(EOutputStatus::SPENT);
			outputsToUpdate.push_back(outputData);
		}
	}

	m_walletDB.AddOutputs(wallet.GetUsername(), masterSeed, outputsToUpdate);
	m_walletDB.UpdateRefreshBlockHeight(wallet.GetUsername(), lastConfirmedHeight);
}

void WalletRefresher::RefreshTransactions(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& refreshedOutputs, std::vector<WalletTx>& walletTransactions)
{
	std::unordered_map<uint32_t, WalletTx> walletTransactionsById;
	for (WalletTx& walletTx : walletTransactions)
	{
		walletTransactionsById.insert({ walletTx.GetId(), walletTx });
	}

	for (const OutputData& output : refreshedOutputs)
	{
		if (output.GetWalletTxId().has_value())
		{
			auto iter = walletTransactionsById.find(output.GetWalletTxId().value());
			if (iter != walletTransactionsById.cend())
			{
				WalletTx walletTx = iter->second;

				if (output.GetBlockHeight().has_value())
				{
					walletTx.SetConfirmedHeight(output.GetBlockHeight().value());

					if (walletTx.GetType() == EWalletTxType::RECEIVING_IN_PROGRESS)
					{
						LoggerAPI::LogDebug("WalletRefresher::RefreshTransactions - Marking transaction as received: " + walletTx.GetId());

						walletTx.SetType(EWalletTxType::RECEIVED);
						m_walletDB.AddTransaction(username, masterSeed, walletTx);
					}
					else if (walletTx.GetType() == EWalletTxType::SENDING_FINALIZED)
					{
						LoggerAPI::LogDebug("WalletRefresher::RefreshTransactions - Marking transaction as sent: " + walletTx.GetId());

						walletTx.SetType(EWalletTxType::SENT);
						m_walletDB.AddTransaction(username, masterSeed, walletTx);
					}
				}
			}
		}
	}

	for (WalletTx& walletTx : walletTransactions)
	{
		if (walletTx.GetTransaction().has_value() && walletTx.GetType() == EWalletTxType::SENDING_FINALIZED)
		{
			const std::vector<TransactionOutput>& outputs = walletTx.GetTransaction().value().GetBody().GetOutputs();
			for (const TransactionOutput& output : outputs)
			{
				std::unique_ptr<OutputData> pOutputData = FindOutput(refreshedOutputs, output.GetCommitment());
				if (pOutputData != nullptr)
				{
					if (pOutputData->GetStatus() == EOutputStatus::SPENT)
					{
						LoggerAPI::LogDebug("WalletRefresher::RefreshTransactions - Output is spent. Marking transaction as sent: " + walletTx.GetId());

						walletTx.SetType(EWalletTxType::SENT);
						m_walletDB.AddTransaction(username, masterSeed, walletTx);
					}

					break;
				}
			}
		}
	}
}

std::unique_ptr<OutputData> WalletRefresher::FindOutput(const std::vector<OutputData>& walletOutputs, const Commitment& commitment) const
{
	for (const OutputData& outputData : walletOutputs)
	{
		if (commitment == outputData.GetOutput().GetCommitment())
		{
			return std::make_unique<OutputData>(OutputData(outputData));
		}
	}

	return std::unique_ptr<OutputData>(nullptr);
}