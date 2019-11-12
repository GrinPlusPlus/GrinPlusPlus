#include "CancelTx.h"

#include <Infrastructure/Logger.h>
#include <Core/Exceptions/WalletException.h>

void CancelTx::CancelWalletTx(const SecureVector& masterSeed, Locked<IWalletDB> walletDB, WalletTx& walletTx)
{
	const EWalletTxType type = walletTx.GetType();
	WALLET_DEBUG_F("Canceling WalletTx (%lu) of type (%s).", walletTx.GetId(), WalletTxType::ToString(type));
	if (type == EWalletTxType::RECEIVING_IN_PROGRESS)
	{
		walletTx.SetType(EWalletTxType::RECEIVED_CANCELED);
	}
	else if (type == EWalletTxType::SENDING_STARTED)
	{
		walletTx.SetType(EWalletTxType::SENT_CANCELED);
	}
	else
	{
		WALLET_ERROR("WalletTx was not in a cancelable status.");
		throw WALLET_EXCEPTION("WalletTx not in a cancelable status.");
	}

	std::unordered_set<Commitment> inputCommitments;
	std::optional<Transaction> transactionOpt = walletTx.GetTransaction();
	if (walletTx.GetType() == EWalletTxType::SENT_CANCELED && transactionOpt.has_value())
	{
		for (const TransactionInput& input : transactionOpt.value().GetBody().GetInputs())
		{
			inputCommitments.insert(input.GetCommitment());
		}
	}

	std::vector<OutputData> outputs = walletDB.Read()->GetOutputs(masterSeed);
	std::vector<OutputData> outputsToUpdate = GetOutputsToUpdate(outputs, inputCommitments, walletTx);

	auto pBatch = walletDB.BatchWrite();
	pBatch->AddOutputs(masterSeed, outputsToUpdate);
	pBatch->AddTransaction(masterSeed, walletTx);
	pBatch->Commit();
}

std::vector<OutputData> CancelTx::GetOutputsToUpdate(
	std::vector<OutputData>& outputs,
	const std::unordered_set<Commitment>& inputCommitments,
	const WalletTx& walletTx)
{
	std::vector<OutputData> outputsToUpdate;
	for (OutputData& output : outputs)
	{
		const auto iter = inputCommitments.find(output.GetOutput().GetCommitment());
		if (iter != inputCommitments.end())
		{
			const EOutputStatus status = output.GetStatus();
			WALLET_DEBUG_F("Found input with status (%s)", OutputStatus::ToString(status));

			if (status == EOutputStatus::NO_CONFIRMATIONS || status == EOutputStatus::SPENT)
			{
				output.SetStatus(EOutputStatus::CANCELED);
				outputsToUpdate.push_back(output);
			}
			else if (status == EOutputStatus::LOCKED)
			{
				output.SetStatus(EOutputStatus::SPENDABLE);
				outputsToUpdate.push_back(output);
			}
			else if (walletTx.GetType() != EWalletTxType::SENT_CANCELED)
			{
				WALLET_ERROR_F("Can't cancel output with status (%s)", OutputStatus::ToString(status));
				throw WALLET_EXCEPTION("Output not in a cancelable status.");
			}
		}
		else if (output.GetWalletTxId() == walletTx.GetId())
		{
			const EOutputStatus status = output.GetStatus();
			WALLET_DEBUG_F("Found output with status (%s)", OutputStatus::ToString(status));

			if (status == EOutputStatus::NO_CONFIRMATIONS || status == EOutputStatus::SPENT)
			{
				output.SetStatus(EOutputStatus::CANCELED);
				outputsToUpdate.push_back(output);
			}
			else if (status != EOutputStatus::CANCELED)
			{
				WALLET_ERROR_F("Can't cancel output with status (%s)", OutputStatus::ToString(status));
				throw WALLET_EXCEPTION("Output not in a cancelable status.");
			}
		}
	}

	return outputsToUpdate;
}