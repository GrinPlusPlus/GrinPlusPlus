#include <TxBuilder.h>
#include <Core/Util/FeeUtil.h>
#include <Crypto/CSPRNG.h>
#include <Crypto/Crypto.h>
#include <Crypto/Hasher.h>

Test::Tx TxBuilder::BuildCoinbaseTx(const KeyChainPath& keyChainPath, const uint64_t amount)
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

Transaction TxBuilder::BuildTx(const Criteria& criteria)
{
    BlindingFactor txOffset = CSPRNG::GenerateRandom32();
    const uint64_t fee = FeeUtil::CalculateFee(
        criteria.fee_base,
        criteria.inputs.size(),
        criteria.outputs.size() + (criteria.include_change ? 1 : 0),
        criteria.num_kernels
    );

    // Calculate sum inputs blinding factors xI.
    std::vector<BlindingFactor> input_blinds;
    std::transform(
        criteria.inputs.begin(), criteria.inputs.end(),
        std::back_inserter(input_blinds),
        [this](const Test::Input& input) -> BlindingFactor {
            return BlindingFactor(m_keyChain.DerivePrivateKey(input.path, input.amount).GetBytes());
        }
    );

    std::vector<BlindingFactor> output_blinds;
    std::vector<TransactionOutput> outputs;
    for (const auto& output : criteria.outputs) {
        const auto builtOutput = BuildOutput(output, EOutputFeatures::DEFAULT);
        output_blinds.push_back(builtOutput.first);
        outputs.push_back(builtOutput.second);
    }

    // Calculate Change
    if (criteria.include_change) {
        const uint64_t input_sum = std::accumulate(
            criteria.inputs.begin(), criteria.inputs.end(), (uint64_t)0,
            [](uint64_t sum, const Test::Input& input) { return sum + input.amount; }
        );
        const uint64_t output_sum = std::accumulate(
            criteria.outputs.begin(), criteria.outputs.end(), (uint64_t)0,
            [](uint64_t sum, const Test::Output& output) { return sum + output.amount; }
        );

        const uint64_t change_amount = input_sum - (output_sum + fee);
        const auto change_output = BuildOutput(
            Test::Output{ KeyChainPath({2, 0}), change_amount },
            EOutputFeatures::DEFAULT
        );
        output_blinds.push_back(change_output.first);
        outputs.push_back(change_output.second);
    }

    // Calculate total kernel excess sum for all inputs and outputs.
    BlindingFactor total_excess = Crypto::AddBlindingFactors(output_blinds, input_blinds);

    //
    // Build kernels
    //
    uint64_t remaining_fee = fee;
    BlindingFactor remaining_blind = Crypto::AddBlindingFactors({ total_excess }, { txOffset });
    std::vector<TransactionKernel> kernels;
    for (uint64_t i = 1; i <= criteria.num_kernels; i++) {
        uint64_t kernel_fee = remaining_fee;
        BlindingFactor kernel_blind = remaining_blind;
        if (i != criteria.num_kernels) {
            kernel_fee /= 2;
            remaining_fee -= kernel_fee;

            kernel_blind = CSPRNG::GenerateRandom32();
            remaining_blind = Crypto::AddBlindingFactors({ remaining_blind }, { kernel_blind });
        }

        TransactionKernel kernel = BuildKernel(
            EKernelFeatures::DEFAULT_KERNEL,
            Fee::From(kernel_fee),
            kernel_blind,
            Crypto::CommitBlinded(0, kernel_blind)
        );

        kernels.push_back(std::move(kernel));
    }

    std::vector<TransactionInput> inputs;
    for (const auto& input : criteria.inputs) {
        inputs.push_back(input.input);
    }

    return Transaction(
        std::move(txOffset),
        TransactionBody(std::move(inputs), std::move(outputs), std::move(kernels))
    );
}

TransactionKernel TxBuilder::BuildKernel(
    const EKernelFeatures features,
    const Fee& fee,
    const BlindingFactor& blind,
    const Commitment& commit)
{
    Serializer serializer;
    serializer.Append<uint8_t>((uint8_t)features);

    if (features != EKernelFeatures::COINBASE_KERNEL) {
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

std::pair<BlindingFactor, TransactionOutput> TxBuilder::BuildOutput(
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

Test::Input TxBuilder::BuildInput(
    EOutputFeatures features,
    KeyChainPath path,
    uint64_t amount)
{
    TransactionOutput output = BuildOutput(Test::Output{ path, amount }, features).second;
    return Test::Input{
        TransactionInput(features, output.GetCommitment()),
        std::move(path),
        amount
    };
}