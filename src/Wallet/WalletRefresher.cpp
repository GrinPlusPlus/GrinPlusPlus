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

    if (m_pNodeClient->GetChainHeight() < pBatch->GetRefreshBlockHeight()) {
        WALLET_TRACE("Skipping refresh since node is resyncing.");
        return std::vector<OutputDataEntity>();
    }

    std::vector<OutputDataEntity> walletOutputs = pBatch->GetOutputs(masterSeed);
    std::vector<WalletTx> walletTransactions = pBatch->GetTransactions(masterSeed);

    // 1. Check for new outputs we hadn't seen before.
    std::vector<OutputDataEntity> new_outputs = FindNewOutputs(masterSeed, pBatch.GetShared(), walletOutputs, fromGenesis);

    // 2. Create a new WalletTx for each newly received output, and add the output & tx to the database.
    for (OutputDataEntity& new_output : new_outputs) {
        WALLET_INFO_F("Restoring unknown output: {}", new_output);

        auto blockTimeOpt = GetBlockTime(new_output);

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
            new_output.GetBlockHeight(),
            new_output.GetAmount(),
            0,
            std::nullopt,
            std::nullopt,
            std::nullopt
        );

        new_output.SetWalletTxId(walletTxId);
        pBatch->AddOutputs(masterSeed, std::vector<OutputDataEntity>({ new_output }));
        pBatch->AddTransaction(masterSeed, walletTx);

        walletOutputs.push_back(new_output);
        walletTransactions.emplace_back(std::move(walletTx));
    }

    // 3. Refresh status for all wallet outputs
    RefreshOutputs(masterSeed, pBatch, walletOutputs);

    // 4. For all wallet outputs, update matching WalletTx status.
    RefreshTransactions(masterSeed, pBatch, walletOutputs, walletTransactions);

    pBatch->Commit();
    return walletOutputs;
}

std::vector<OutputDataEntity> WalletRefresher::FindNewOutputs(
    const SecureVector& masterSeed,
    const std::shared_ptr<IWalletDB>& pBatch,
    const std::vector<OutputDataEntity>& walletOutputs,
    const bool fromGenesis)
{
    std::vector<OutputDataEntity> new_outputs;

    KeyChain keyChain = KeyChain::FromSeed(masterSeed);
    OutputRestorer restorer(m_pNodeClient, keyChain);
    std::vector<OutputDataEntity> restoredOutputs = restorer.FindAndRewindOutputs(pBatch, fromGenesis);

    // For each restored output, look for known outputs with matching commitment.
    // If no matching output is found, this must be a new output that was received by another one of the user's wallets with the same seed.
    for (OutputDataEntity& restoredOutput : restoredOutputs) {
        WALLET_INFO_F("Output found at index {}", restoredOutput.GetMMRIndex().value_or(0));

        if (restoredOutput.GetStatus() != EOutputStatus::SPENT) {
            auto pExistingOutput = FindOutput(walletOutputs, restoredOutput.GetCommitment());
            if (pExistingOutput == nullptr) {
                new_outputs.push_back(restoredOutput);
            }
        }
    }

    return new_outputs;
}

void WalletRefresher::RefreshOutputs(const SecureVector& masterSeed, Writer<IWalletDB> pBatch, std::vector<OutputDataEntity>& walletOutputs)
{
    std::vector<Commitment> commitments;
    std::transform(
        walletOutputs.begin(), walletOutputs.end(),
        std::back_inserter(commitments),
        [](const OutputDataEntity& output) { return output.GetCommitment(); }
    );

    const uint64_t lastConfirmedHeight = m_pNodeClient->GetChainHeight();

    const std::map<Commitment, OutputLocation> outputLocations = m_pNodeClient->GetOutputsByCommitment(commitments);
    for (OutputDataEntity& outputData : walletOutputs) {
        const EOutputStatus current_status = outputData.GetStatus();

        auto iter = outputLocations.find(outputData.GetCommitment());
        if (iter != outputLocations.cend()) {
            // Output is on chain, so unless it's being spent, then we can mark it as spendable.
            if (current_status == EOutputStatus::LOCKED) {
                continue;
            }

            // If spendable status, it's already up-to-date.
            if (current_status == EOutputStatus::SPENDABLE) {
                continue;
            }

            WALLET_DEBUG_F("Marking output as spendable: {}", outputData);
            outputData.SetBlockHeight(iter->second.GetBlockHeight());
            outputData.SetStatus(EOutputStatus::SPENDABLE);
            pBatch->SaveOutput(masterSeed, outputData);
        } else {
            // NO_CONFIRMATIONS means the output is being received, but hasn't been seen on chain yet.
            // Since we still have not seen it on-chain, no need to update.
            if (current_status == EOutputStatus::NO_CONFIRMATIONS) {
                continue;
            }

            // Output has already been marked as SPENT or CANCELED, so no need to update it.
            if (current_status == EOutputStatus::SPENT || current_status == EOutputStatus::CANCELED) {
                continue;
            }

            // TODO: Check if (lastConfirmedHeight > outputData.GetConfirmedHeight)
            WALLET_DEBUG_F("Marking output as spent: {}", outputData);

            outputData.SetStatus(EOutputStatus::SPENT);
            pBatch->SaveOutput(masterSeed, outputData);
        }
    }

    pBatch->UpdateRefreshBlockHeight(lastConfirmedHeight);
}

void WalletRefresher::RefreshTransactions(
    const SecureVector& masterSeed,
    Writer<IWalletDB> pBatch,
    const std::vector<OutputDataEntity>& refreshedOutputs,
    std::vector<WalletTx>& walletTransactions)
{
    std::unordered_map<uint32_t, WalletTx> walletTransactionsById;
    for (WalletTx& walletTx : walletTransactions) {
        walletTransactionsById.insert({ walletTx.GetId(), walletTx });
    }

    for (const OutputDataEntity& output : refreshedOutputs) {
        if (output.GetWalletTxId().has_value()) {
            auto iter = walletTransactionsById.find(output.GetWalletTxId().value());
            if (iter != walletTransactionsById.cend()) {
                WalletTx walletTx = iter->second;

                if (output.GetBlockHeight().has_value()) {
                    walletTx.SetConfirmedHeight(output.GetBlockHeight().value());

                    if (walletTx.GetType() == EWalletTxType::RECEIVING_IN_PROGRESS) {
                        WALLET_DEBUG_F("Marking transaction as received: {}", walletTx.GetId());

                        walletTx.SetType(EWalletTxType::RECEIVED);
                        pBatch->AddTransaction(masterSeed, walletTx);
                    } else if (walletTx.GetType() == EWalletTxType::SENDING_FINALIZED) {
                        WALLET_DEBUG_F("Marking transaction as sent: {}", walletTx.GetId());

                        walletTx.SetType(EWalletTxType::SENT);
                        pBatch->AddTransaction(masterSeed, walletTx);
                    }
                }
            }
        }
    }

    for (WalletTx& walletTx : walletTransactions) {
        const auto& tx = walletTx.GetTransaction();
        if (tx.has_value() && walletTx.GetType() == EWalletTxType::SENDING_FINALIZED) {
            for (const TransactionOutput& output : tx.value().GetOutputs()) {
                auto pOutputData = FindOutput(refreshedOutputs, output.GetCommitment());
                if (pOutputData != nullptr) {
                    if (pOutputData->GetStatus() == EOutputStatus::SPENT) {
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
    if (output.GetBlockHeight().has_value()) {
        auto pHeader = m_pNodeClient->GetBlockHeader(output.GetBlockHeight().value());
        if (pHeader != nullptr) {
            return std::make_optional(TimeUtil::ToTimePoint(pHeader->GetTimestamp() * 1000));
        }
    }

    return std::nullopt;
}

std::unique_ptr<OutputDataEntity> WalletRefresher::FindOutput(
    const std::vector<OutputDataEntity>& walletOutputs,
    const Commitment& commitment) const
{
    for (const OutputDataEntity& outputData : walletOutputs) {
        if (commitment == outputData.GetCommitment()) {
            return std::make_unique<OutputDataEntity>(outputData);
        }
    }

    return std::unique_ptr<OutputDataEntity>(nullptr);
}