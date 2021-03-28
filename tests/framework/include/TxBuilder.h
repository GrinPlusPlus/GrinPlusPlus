#pragma once

#include <Models/TxModels.h>

#include <Core/Models/Transaction.h>
#include <Wallet/Keychain/KeyChain.h>
#include <Crypto/CSPRNG.h>
#include <Crypto/Crypto.h>
#include <Crypto/Hasher.h>
#include <tuple>

class TxBuilder
{
    static const uint64_t BASE_REWARD = 60'000'000'000;
public:
    TxBuilder(const KeyChain& keyChain) : m_keyChain(keyChain) { }

    Test::Tx BuildCoinbaseTx(const KeyChainPath& keyChainPath, const uint64_t amount = BASE_REWARD)
    {
        auto txOutput = BuildOutput({ keyChainPath, amount }, EOutputFeatures::COINBASE_OUTPUT);

        BlindingFactor txOffset(Hash::ValueOf(0));
        Commitment kernelCommitment = Crypto::AddCommitments(
            { txOutput.second.GetCommitment() },
            { Crypto::CommitTransparent(amount) }
        );

        TransactionKernel kernel = BuildKernel(
            EKernelFeatures::COINBASE_KERNEL,
            Fee(),
            Crypto::AddBlindingFactors({ txOutput.first }, { txOffset }),
            kernelCommitment
        );

        TransactionPtr pTransaction = std::make_shared<Transaction>(
            std::move(txOffset),
            TransactionBody({}, { txOutput.second }, { kernel })
        );

        return Test::Tx({ pTransaction, {}, { { keyChainPath, amount } } });
    }

    Transaction BuildTx(
        const Fee& fee,
        const std::vector<Test::Input>& inputs,
        const std::vector<Test::Output>& outputs)
    {
        BlindingFactor txOffset = CSPRNG::GenerateRandom32();

        // Calculate sum inputs blinding factors xI.
        std::vector<BlindingFactor> inputBlindingFactors;
        std::transform(
            inputs.begin(), inputs.end(),
            std::back_inserter(inputBlindingFactors),
            [this](const Test::Input& input) -> BlindingFactor {
                return BlindingFactor(m_keyChain.DerivePrivateKey(input.path, input.amount).GetBytes());
            }
        );

        BlindingFactor inputBFSum = Crypto::AddBlindingFactors(inputBlindingFactors, {});

        std::vector<BlindingFactor> outputBlindingFactors;
        std::vector<TransactionOutput> builtOutputs;
        for (const auto& output : outputs)
        {
            const auto builtOutput = BuildOutput(output, EOutputFeatures::DEFAULT);
            outputBlindingFactors.push_back(builtOutput.first);
            builtOutputs.push_back(builtOutput.second);
        }

        // Calculate sum change outputs blinding factors xC.
        BlindingFactor outputBFSum = Crypto::AddBlindingFactors(outputBlindingFactors, {});

        // Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
        BlindingFactor totalExcess = Crypto::AddBlindingFactors({ outputBFSum }, { inputBFSum });

        // Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
        BlindingFactor privateKeyBF = Crypto::AddBlindingFactors({ totalExcess }, { txOffset });

        TransactionKernel kernel = BuildKernel(
            EKernelFeatures::DEFAULT_KERNEL,
            fee,
            privateKeyBF,
            Crypto::CommitBlinded(0, privateKeyBF)
        );

        std::vector<TransactionInput> txInputs;
        for (const auto& input : inputs)
        {
            txInputs.push_back(input.input);
        }

        return Transaction(
            std::move(txOffset),
            TransactionBody(std::move(txInputs), std::move(builtOutputs), { kernel } )
        );
    }

private:
    const KeyChain& m_keyChain;

    TransactionKernel BuildKernel(
        const EKernelFeatures features,
        const Fee& fee,
        const BlindingFactor& blind,
        const Commitment& commit)
    {
        Serializer serializer;
        serializer.Append<uint8_t>((uint8_t)features);

        if (features != EKernelFeatures::COINBASE_KERNEL)
        {
            fee.Serialize(serializer);
        }

        auto pSignature = Crypto::BuildCoinbaseSignature(
            BlindingFactor(blind).ToSecretKey(),
            commit,
            Hasher::Blake2b(serializer.GetBytes())
        );

        return TransactionKernel(
            features,
            fee,
            0,
            Commitment(commit),
            Signature(*pSignature)
        );
    }

    std::pair<BlindingFactor, TransactionOutput> BuildOutput(
        const Test::Output& output,
        const EOutputFeatures& features)
    {
        SecretKey blindingFactor = m_keyChain.DerivePrivateKey(output.path, output.amount);

        Commitment commitment = Crypto::CommitBlinded(
            output.amount,
            BlindingFactor(blindingFactor.GetBytes())
        );

        RangeProof rangeProof = m_keyChain.GenerateRangeProof(
            output.path,
            output.amount,
            commitment,
            blindingFactor,
            EBulletproofType::ENHANCED
        );

        return std::make_pair(
            BlindingFactor(blindingFactor.GetBytes()),
            TransactionOutput(
                features,
                std::move(commitment),
                std::move(rangeProof)
            )
        );
    }
};