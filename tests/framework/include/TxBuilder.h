#pragma once

#include <Models/TxModels.h>

#include <Consensus.h>
#include <Core/Models/Transaction.h>
#include <Wallet/Keychain/KeyChain.h>
#include <tuple>

class TxBuilder
{
public:
    TxBuilder(const KeyChain& keyChain) : m_keyChain(keyChain) { }

    Test::Tx BuildCoinbaseTx(
        const KeyChainPath& keyChainPath,
        const uint64_t amount = Consensus::REWARD
    );

    struct Criteria
    {
        std::vector<Test::Input> inputs;
        std::vector<Test::Output> outputs;
        bool include_change{ false };
        uint64_t fee_base{ 500'000 };
        uint64_t num_kernels{ 1 };
    };
    Transaction BuildTx(const Criteria& criteria);

    TransactionKernel BuildKernel(
        const EKernelFeatures features,
        const Fee& fee,
        const BlindingFactor& blind,
        const Commitment& commit
    );

    std::pair<BlindingFactor, TransactionOutput> BuildOutput(
        const Test::Output& output,
        const EOutputFeatures& features
    );
    
    Test::Input BuildInput(
        EOutputFeatures features,
        KeyChainPath path,
        uint64_t amount
    );

private:
    KeyChain m_keyChain;
};