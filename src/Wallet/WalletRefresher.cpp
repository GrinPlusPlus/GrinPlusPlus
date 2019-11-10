#include "WalletRefresher.h"
#include "OutputRestorer.h"

#include <Infrastructure/Logger.h>
#include <Wallet/WalletUtil.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <unordered_map>

WalletRefresher::WalletRefresher(const Config& config, INodeClientConstPtr pNodeClient, IWalletDBPtr pWalletDB)
	: m_config(config), m_pNodeClient(pNodeClient), m_pWalletDB(pWalletDB)
{

}

// TODO: Rewrite this
// 0. Initial login after upgrade - for every output, find matching WalletTx and update OutputData TxId. If none found, create new WalletTx.
//
// 1. Check for own outputs in new blocks.
// 2. For each output, look for OutputData with matching commitment. If no output found, create new WalletTx and OutputData.
// 3. Refresh status for all OutputData by calling m_pNodeClient->GetOutputsByCommitment
// 4. For all OutputData, update matching WalletTx status.

std::vector<OutputData> WalletRefresher::Refresh(const SecureVector& masterSeed, Wallet& wallet, const bool fromGenesis)
{
	WALLET_INFO_F("Refreshing wallet for user: %s", wallet.GetUsername());

	if (m_pNodeClient->GetChainHeight() < m_pWalletDB->GetRefreshBlockHeight(wallet.GetUsername()))
	{
		WALLET_INFO("Skipping refresh since node is resyncing.");
		return std::vector<OutputData>();
	}

	std::vector<OutputData> walletOutputs = m_pWalletDB->GetOutputs(wallet.GetUsername(), masterSeed);
	std::vector<WalletTx> walletTransactions = m_pWalletDB->GetTransactions(wallet.GetUsername(), masterSeed);

	// 1. Check for own outputs in new blocks.
	KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);
	std::vector<OutputData> restoredOutputs = OutputRestorer(m_config, m_pNodeClient, keyChain).FindAndRewindOutputs(masterSeed, wallet, fromGenesis);

	// 2. For each restored output, look for OutputData with matching commitment.
	for (OutputData& restoredOutput : restoredOutputs)
	{
		WALLET_INFO_F("Output found at index %llu", restoredOutput.GetMMRIndex().value_or(0));

		if (restoredOutput.GetStatus() != EOutputStatus::SPENT)
		{
			const Commitment& commitment = restoredOutput.GetOutput().GetCommitment();
			std::unique_ptr<OutputData> pExistingOutput = FindOutput(walletOutputs, commitment);
			if (pExistingOutput == nullptr)
			{
				WALLET_INFO_F("Restoring unknown output with commitment: %s", commitment);

				auto blockTimeOpt = GetBlockTime(restoredOutput);

				// If no output found, create new WalletTx and OutputData.
				const uint32_t walletTxId = wallet.GetNextWalletTxId();
				WalletTx walletTx(
					walletTxId,
					EWalletTxType::RECEIVED,
					std::nullopt,
					std::nullopt,
					std::nullopt,
					blockTimeOpt.value_or(std::chrono::system_clock::now()),
					blockTimeOpt,
					restoredOutput.GetBlockHeight(),
					restoredOutput.GetAmount(),
					0,
					std::nullopt,
					std::nullopt
				);

				restoredOutput.SetWalletTxId(walletTxId);
				m_pWalletDB->AddOutputs(wallet.GetUsername(), masterSeed, std::vector<OutputData>({ restoredOutput }));
				m_pWalletDB->AddTransaction(wallet.GetUsername(), masterSeed, walletTx);

				walletOutputs.push_back(restoredOutput);
				walletTransactions.emplace_back(std::move(walletTx));
			}
			//else if (pExistingOutput->GetStatus() == EOutputStatus::SPENT || pExistingOutput->GetStatus() == EOutputStatus::CANCELED)
			//{

			//}
		}
	}

	// 3. Refresh status for all OutputData by calling m_pNodeClient->GetOutputsByCommitment
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
		WALLET_DEBUG_F("Refreshing output with commitment: %s", commitment);
		commitments.push_back(commitment);		
	}

	const uint64_t lastConfirmedHeight = m_pNodeClient->GetChainHeight();

	std::vector<OutputData> outputsToUpdate;
	const std::map<Commitment, OutputLocation> outputLocations = m_pNodeClient->GetOutputsByCommitment(commitments);
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
						WALLET_DEBUG_F("Marking output as immature: %s", outputData);

						outputData.SetBlockHeight(outputBlockHeight);
						outputData.SetStatus(EOutputStatus::IMMATURE);
						outputsToUpdate.push_back(outputData);
					}
				}
				else if (outputData.GetStatus() != EOutputStatus::SPENDABLE)
				{
					WALLET_DEBUG_F("Marking output as spendable: %s", outputData);

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
			WALLET_DEBUG_F("Marking output as spent: %s", outputData);

			outputData.SetStatus(EOutputStatus::SPENT);
			outputsToUpdate.push_back(outputData);
		}
	}

	m_pWalletDB->AddOutputs(wallet.GetUsername(), masterSeed, outputsToUpdate);
	m_pWalletDB->UpdateRefreshBlockHeight(wallet.GetUsername(), lastConfirmedHeight);
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
						WALLET_DEBUG_F("Marking transaction as received: %lu", walletTx.GetId());

						walletTx.SetType(EWalletTxType::RECEIVED);
						m_pWalletDB->AddTransaction(username, masterSeed, walletTx);
					}
					else if (walletTx.GetType() == EWalletTxType::SENDING_FINALIZED)
					{
						WALLET_DEBUG_F("Marking transaction as sent: %lu", walletTx.GetId());

						walletTx.SetType(EWalletTxType::SENT);
						m_pWalletDB->AddTransaction(username, masterSeed, walletTx);
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
						WALLET_DEBUG_F("Output is spent. Marking transaction as sent: %lu", walletTx.GetId());

						walletTx.SetType(EWalletTxType::SENT);
						m_pWalletDB->AddTransaction(username, masterSeed, walletTx);
					}

					break;
				}
			}
		}
	}
}

std::optional<std::chrono::system_clock::time_point> WalletRefresher::GetBlockTime(const OutputData& output) const
{
	if (output.GetBlockHeight().has_value())
	{
		std::unique_ptr<BlockHeader> pHeader = m_pNodeClient->GetBlockHeader(output.GetBlockHeight().value());
		if (pHeader != nullptr)
		{
			return std::make_optional(TimeUtil::ToTimePoint(pHeader->GetTimestamp()));
		}
	}

	return std::nullopt;
}

std::unique_ptr<OutputData> WalletRefresher::FindOutput(const std::vector<OutputData>& walletOutputs, const Commitment& commitment) const
{
	for (const OutputData& outputData : walletOutputs)
	{
		if (commitment == outputData.GetOutput().GetCommitment())
		{
			return std::make_unique<OutputData>(outputData);
		}
	}

	return std::unique_ptr<OutputData>(nullptr);
}