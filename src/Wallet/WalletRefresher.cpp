#include "WalletRefresher.h"
#include "OutputRestorer.h"
#include <Wallet/Keychain/KeyChain.h>

#include <Common/Logger.h>
#include <Wallet/WalletUtil.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <unordered_map>

// TODO: Rewrite this && use cache - Shouldn't refresh when block height = same
// 0. Initial login after upgrade - for every output, find matching WalletTx and update OutputDataEntity TxId. If none found, create new WalletTx.
//
// 1. Check for own outputs in new blocks.
// 2. For each output, look for OutputDataEntity with matching commitment. If no output found, create new WalletTx and OutputDataEntity.
// 3. Refresh status for all OutputDataEntity by calling m_pNodeClient->GetOutputsByCommitment
// 4. For all OutputDataEntity, update matching WalletTx status.

std::vector<OutputDataEntity> WalletRefresher::Refresh(const SecureVector& masterSeed, Locked<IWalletDB> walletDB, const bool fromGenesis)
{
	auto pBatch = walletDB.BatchWrite();

	if (m_pNodeClient->GetChainHeight() < pBatch->GetRefreshBlockHeight())
	{
		WALLET_TRACE("Skipping refresh since node is resyncing.");
		return std::vector<OutputDataEntity>();
	}

	std::vector<OutputDataEntity> walletOutputs = pBatch->GetOutputs(masterSeed);
	std::vector<WalletTx> walletTransactions = pBatch->GetTransactions(masterSeed);

	// 1. Check for own outputs in new blocks.
	KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);
	OutputRestorer restorer(m_config, m_pNodeClient, keyChain);
	std::vector<OutputDataEntity> restoredOutputs = restorer.FindAndRewindOutputs(pBatch.GetShared(), fromGenesis);

	// 2. For each restored output, look for OutputDataEntity with matching commitment.
	for (OutputDataEntity& restoredOutput : restoredOutputs)
	{
		WALLET_INFO_F("Output found at index {}", restoredOutput.GetMMRIndex().value_or(0));

		if (restoredOutput.GetStatus() != EOutputStatus::SPENT)
		{
			const Commitment& commitment = restoredOutput.GetOutput().GetCommitment();
			std::unique_ptr<OutputDataEntity> pExistingOutput = FindOutput(walletOutputs, commitment);
			if (pExistingOutput == nullptr)
			{
				WALLET_INFO_F("Restoring unknown output with commitment: {}", commitment);

				auto blockTimeOpt = GetBlockTime(restoredOutput);

				// If no output found, create new WalletTx and OutputDataEntity.
				const uint32_t walletTxId = pBatch->GetNextTransactionId();
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
					std::nullopt,
					std::nullopt
				);

				restoredOutput.SetWalletTxId(walletTxId);
				pBatch->AddOutputs(masterSeed, std::vector<OutputDataEntity>({ restoredOutput }));
				pBatch->AddTransaction(masterSeed, walletTx);

				walletOutputs.push_back(restoredOutput);
				walletTransactions.emplace_back(std::move(walletTx));
			}
		}
	}

	// 3. Refresh status for all OutputDataEntity by calling m_pNodeClient->GetOutputsByCommitment
	RefreshOutputs(masterSeed, pBatch, walletOutputs);

	// 4. For all OutputDataEntity, update matching WalletTx status.
	RefreshTransactions(masterSeed, pBatch, walletOutputs, walletTransactions);

	pBatch->Commit();
	return walletOutputs;
}

void WalletRefresher::RefreshOutputs(const SecureVector& masterSeed, Writer<IWalletDB> pBatch, std::vector<OutputDataEntity>& walletOutputs)
{
	std::vector<Commitment> commitments;

	for (const OutputDataEntity& outputData : walletOutputs)
	{
		const Commitment& commitment = outputData.GetOutput().GetCommitment();

		/*if (outputData.GetStatus() == EOutputStatus::SPENT)// || outputData.GetStatus() == EOutputStatus::CANCELED)
		{
			WALLET_TRACE_F("No need to refresh spent/canceled output with commitment: " + commitment);
			continue;
		}*/

		// TODO: What if commitment has mmr_index?
		commitments.push_back(commitment);		
	}

	const uint64_t lastConfirmedHeight = m_pNodeClient->GetChainHeight();

	std::vector<OutputDataEntity> outputsToUpdate;
	const std::map<Commitment, OutputLocation> outputLocations = m_pNodeClient->GetOutputsByCommitment(commitments);
	for (OutputDataEntity& outputData : walletOutputs)
	{
		auto iter = outputLocations.find(outputData.GetOutput().GetCommitment());
		if (iter != outputLocations.cend())
		{
			if (outputData.GetStatus() != EOutputStatus::LOCKED)
			{
				const EOutputFeatures features = outputData.GetOutput().GetFeatures();
				const uint64_t outputBlockHeight = iter->second.GetBlockHeight();
				const uint32_t minimumConfirmations = m_config.GetMinimumConfirmations();
				const bool immature = WalletUtil::IsOutputImmature(
					features,
					outputBlockHeight,
					lastConfirmedHeight,
					minimumConfirmations
				);

				if (immature)
				{
					if (outputData.GetStatus() != EOutputStatus::IMMATURE)
					{
						WALLET_DEBUG_F("Marking output as immature: {}", outputData);

						outputData.SetBlockHeight(outputBlockHeight);
						outputData.SetStatus(EOutputStatus::IMMATURE);
						outputsToUpdate.push_back(outputData);
					}
				}
				else if (outputData.GetStatus() != EOutputStatus::SPENDABLE)
				{
					WALLET_DEBUG_F("Marking output as spendable: {}", outputData);

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
			WALLET_DEBUG_F("Marking output as spent: {}", outputData);

			outputData.SetStatus(EOutputStatus::SPENT);
			outputsToUpdate.push_back(outputData);
		}
	}

	pBatch->AddOutputs(masterSeed, outputsToUpdate);
	pBatch->UpdateRefreshBlockHeight(lastConfirmedHeight);
}

void WalletRefresher::RefreshTransactions(
	const SecureVector& masterSeed,
	Writer<IWalletDB> pBatch,
	const std::vector<OutputDataEntity>& refreshedOutputs,
	std::vector<WalletTx>& walletTransactions)
{
	std::unordered_map<uint32_t, WalletTx> walletTransactionsById;
	for (WalletTx& walletTx : walletTransactions)
	{
		walletTransactionsById.insert({ walletTx.GetId(), walletTx });
	}

	for (const OutputDataEntity& output : refreshedOutputs)
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
						WALLET_DEBUG_F("Marking transaction as received: {}", walletTx.GetId());

						walletTx.SetType(EWalletTxType::RECEIVED);
						pBatch->AddTransaction(masterSeed, walletTx);
					}
					else if (walletTx.GetType() == EWalletTxType::SENDING_FINALIZED)
					{
						WALLET_DEBUG_F("Marking transaction as sent: {}", walletTx.GetId());

						walletTx.SetType(EWalletTxType::SENT);
						pBatch->AddTransaction(masterSeed, walletTx);
					}
				}
			}
		}
	}

	for (WalletTx& walletTx : walletTransactions)
	{
		if (walletTx.GetTransaction().has_value() && walletTx.GetType() == EWalletTxType::SENDING_FINALIZED)
		{
			const std::vector<TransactionOutput>& outputs = walletTx.GetTransaction().value().GetOutputs();
			for (const TransactionOutput& output : outputs)
			{
				std::unique_ptr<OutputDataEntity> pOutputData = FindOutput(refreshedOutputs, output.GetCommitment());
				if (pOutputData != nullptr)
				{
					if (pOutputData->GetStatus() == EOutputStatus::SPENT)
					{
						WALLET_DEBUG_F("Output is spent. Marking transaction as sent: {}", walletTx.GetId());

						walletTx.SetType(EWalletTxType::SENT);
						pBatch->AddTransaction(masterSeed, walletTx);
					}

					break;
				}
			}
		}
	}
}

std::optional<std::chrono::system_clock::time_point> WalletRefresher::GetBlockTime(const OutputDataEntity& output) const
{
	if (output.GetBlockHeight().has_value())
	{
		auto pHeader = m_pNodeClient->GetBlockHeader(output.GetBlockHeight().value());
		if (pHeader != nullptr)
		{
			return std::make_optional(TimeUtil::ToTimePoint(pHeader->GetTimestamp() * 1000));
		}
	}

	return std::nullopt;
}

std::unique_ptr<OutputDataEntity> WalletRefresher::FindOutput(
	const std::vector<OutputDataEntity>& walletOutputs,
	const Commitment& commitment) const
{
	for (const OutputDataEntity& outputData : walletOutputs)
	{
		if (commitment == outputData.GetOutput().GetCommitment())
		{
			return std::make_unique<OutputDataEntity>(outputData);
		}
	}

	return std::unique_ptr<OutputDataEntity>(nullptr);
}