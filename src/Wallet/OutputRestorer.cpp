#include "OutputRestorer.h"

#include <Common/Math.h>
#include <Consensus.h>
#include <Wallet/Keychain/KeyChain.h>
#include <Wallet/WalletUtil.h>
#include <Common/Logger.h>

static const uint64_t NUM_OUTPUTS_PER_BATCH = 1000;

std::vector<OutputDataEntity> OutputRestorer::FindAndRewindOutputs(const std::shared_ptr<IWalletDB>& pBatch, const bool fromGenesis) const
{
    uint64_t nextLeafIndex = fromGenesis ? 0 : pBatch->GetRestoreLeafIndex() + 1;
    uint64_t highestIndex = 0;

    std::vector<OutputDataEntity> walletOutputs;
    while (true) {
        std::unique_ptr<OutputRange> pOutputRange = m_pNodeClient->GetOutputsByLeafIndex(nextLeafIndex, NUM_OUTPUTS_PER_BATCH);
        if (pOutputRange == nullptr || pOutputRange->GetLastRetrievedIndex() == 0) {
            // No new outputs since last restore
            return std::vector<OutputDataEntity>();
        }

        for (const OutputDTO& output : pOutputRange->GetOutputs()) {
            std::unique_ptr<OutputDataEntity> pOutputDataEntity = GetWalletOutput(output);
            if (pOutputDataEntity != nullptr) {
                walletOutputs.emplace_back(*pOutputDataEntity);
            }
        }

        if (highestIndex == 0) {
            // Cache this, rather than use the new response from pOutputRange.
            // Otherwise, pOutputRange->GetHighestIndex() could continue to rise slowly during sync, tying up this thread.
            highestIndex = pOutputRange->GetHighestIndex();
        }

        nextLeafIndex = pOutputRange->GetLastRetrievedIndex() + 1;
        if (nextLeafIndex > highestIndex) {
            break;
        }
    }

    pBatch->UpdateRestoreLeafIndex(nextLeafIndex - 1);

    return walletOutputs;
}

std::unique_ptr<OutputDataEntity> OutputRestorer::GetWalletOutput(const OutputDTO& output) const
{
    EBulletproofType type = EBulletproofType::ORIGINAL;

    std::unique_ptr<RewoundProof> pRewoundProof = nullptr;
    if (Consensus::GetHeaderVersion(saturated_sub(output.GetBlockHeight(), Consensus::WEEKS(2))) == 1) {
        pRewoundProof = m_keyChain.RewindRangeProof(output.GetCommitment(), output.GetRangeProof(), EBulletproofType::ORIGINAL);
    }

    if (pRewoundProof == nullptr) {
        type = EBulletproofType::ENHANCED;
        pRewoundProof = m_keyChain.RewindRangeProof(output.GetCommitment(), output.GetRangeProof(), EBulletproofType::ENHANCED);
    }

    if (pRewoundProof) {
        return std::make_unique<OutputDataEntity>(BuildWalletOutput(output, *pRewoundProof, type));
    }

    return nullptr;
}

OutputDataEntity OutputRestorer::BuildWalletOutput(
    const OutputDTO& output,
    const RewoundProof& rewoundProof,
    EBulletproofType type) const
{
    WALLET_INFO_F(
        "Found own output: {} with value {}",
        output.GetCommitment(),
        rewoundProof.GetAmount()
    );

    KeyChainPath keyChainPath(rewoundProof.ToKeyIndices(type));

    SecretKey secret_key;
    if (rewoundProof.GetBlindingFactor() != nullptr && type == EBulletproofType::ORIGINAL) {
        secret_key = *rewoundProof.GetBlindingFactor();
    } else {
        secret_key = m_keyChain.DerivePrivateKey(keyChainPath, rewoundProof.GetAmount());
    }

    return OutputDataEntity{
        KeyChainPath{ rewoundProof.ToKeyIndices(type) },
        std::move(secret_key),
        output.ToTxOutput(),
        rewoundProof.GetAmount(),
        output.IsSpent() ? EOutputStatus::SPENT : EOutputStatus::SPENDABLE,
        output.GetMMRPosition(),
        output.GetBlockHeight(),
        std::nullopt,
        std::vector<std::string>{}
    };
}