#include "CancelTx.h"

#include <Common/Logger.h>
#include <Core/Exceptions/WalletException.h>

void CancelTx::CancelWalletTx(const SecureVector& masterSeed, const std::shared_ptr<IWalletDB>& pWalletDB, WalletTx& walletTx)
{
    const EWalletTxType type = walletTx.GetType();
    WALLET_DEBUG_F("Canceling WalletTx {} of type {}.", walletTx.GetId(), WalletTxType::ToString(type));
    if (type == EWalletTxType::RECEIVING_IN_PROGRESS) {
        walletTx.SetType(EWalletTxType::RECEIVED_CANCELED);
    } else if (type == EWalletTxType::SENDING_STARTED || type == EWalletTxType::SENDING_FINALIZED) {
        walletTx.SetType(EWalletTxType::SENT_CANCELED);
    } else {
        WALLET_ERROR("WalletTx was not in a cancelable status.");
        throw WALLET_EXCEPTION("WalletTx not in a cancelable status.");
    }

    std::unordered_set<Commitment> inputCommitments;
    std::optional<Transaction> transactionOpt = walletTx.GetTransaction();
    if (walletTx.GetType() == EWalletTxType::SENT_CANCELED && transactionOpt.has_value()) {
        for (const TransactionInput& input : transactionOpt.value().GetInputs()) {
            inputCommitments.insert(input.GetCommitment());
        }
    }

    std::vector<OutputDataEntity> outputs = pWalletDB->GetOutputs(masterSeed);
    std::vector<OutputDataEntity> outputsToUpdate = GetOutputsToUpdate(outputs, inputCommitments, walletTx);

    pWalletDB->AddOutputs(masterSeed, outputsToUpdate);
    pWalletDB->AddTransaction(masterSeed, walletTx);
}

std::vector<OutputDataEntity> CancelTx::GetOutputsToUpdate(
    std::vector<OutputDataEntity>& outputs,
    const std::unordered_set<Commitment>& inputCommitments,
    const WalletTx& walletTx)
{
    std::vector<OutputDataEntity> outputsToUpdate;
    for (OutputDataEntity& output : outputs) {
        const auto iter = inputCommitments.find(output.GetOutput().GetCommitment());
        if (iter != inputCommitments.end()) {
            const EOutputStatus status = output.GetStatus();
            WALLET_DEBUG_F("Found input with status ({})", OutputStatus::ToString(status));

            if (status == EOutputStatus::NO_CONFIRMATIONS || status == EOutputStatus::SPENT) {
                output.SetStatus(EOutputStatus::CANCELED);
                outputsToUpdate.push_back(output);
            } else if (status == EOutputStatus::LOCKED) {
                output.SetStatus(EOutputStatus::SPENDABLE);
                outputsToUpdate.push_back(output);
            } else if (walletTx.GetType() != EWalletTxType::SENT_CANCELED) {
                WALLET_ERROR_F("Can't cancel output with status ({})", OutputStatus::ToString(status));
                throw WALLET_EXCEPTION("Output not in a cancelable status.");
            }
        } else if (output.GetWalletTxId() == walletTx.GetId()) {
            const EOutputStatus status = output.GetStatus();
            WALLET_DEBUG_F("Found output with status ({})", OutputStatus::ToString(status));

            if (status == EOutputStatus::NO_CONFIRMATIONS || status == EOutputStatus::SPENT) {
                output.SetStatus(EOutputStatus::CANCELED);
                outputsToUpdate.push_back(output);
            } else if (status != EOutputStatus::CANCELED) {
                WALLET_ERROR_F("Can't cancel output with status ({})", OutputStatus::ToString(status));
                throw WALLET_EXCEPTION("Output not in a cancelable status.");
            }
        }
    }

    return outputsToUpdate;
}