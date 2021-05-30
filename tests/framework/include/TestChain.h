#pragma once

#include <Models/MinedBlock.h>
#include <Models/TxModels.h>
#include <RootCalculator.h>
#include <TestWallet.h>
#include <TxBuilder.h>

#include <BlockChain/BlockChain.h>
#include <Consensus.h>
#include <Common/Util/TimeUtil.h>
#include <Core/Models/FullBlock.h>
#include <Core/Util/TransactionUtil.h>
#include <PMMR/Common/MMRUtil.h>
#include <Wallet/Keychain/KeyChain.h>

class TestChain
{
public:
    TestChain(const IBlockChain::Ptr& pBlockChain) : m_pBlockChain(pBlockChain)
    {
        m_blocks.push_back({ Global::GetGenesisBlock(), Consensus::REWARD, std::nullopt });
    }

    std::vector<MinedBlock> MineChain(const TestWallet::Ptr& pWallet, const uint64_t totalHeight)
    {
        for (size_t i = 1; i <= totalHeight; i++)
        {
            Test::Tx coinbase = pWallet->CreateCoinbase(KeyChainPath::FromString("m/0/0/" + std::to_string(i)), 0);
            MinedBlock block = AddNextBlock({ coinbase });

            if (m_pBlockChain->AddBlock(block.block) != EBlockChainStatus::SUCCESS) {
                throw std::runtime_error("Failed to add block");
            }
        }

        pWallet->RefreshWallet();

        return m_blocks;
    }

    MinedBlock AddNextBlock(const std::vector<Test::Tx>& txs, const uint64_t additionalDifficulty = 0)
    {
        std::vector<TransactionPtr> transactions;
        std::transform(
            txs.cbegin(), txs.cend(),
            std::back_inserter(transactions),
            [](const Test::Tx& tx) { return tx.pTransaction; }
        );
        TransactionPtr pTransaction = TransactionUtil::Aggregate(transactions);

        auto pPrevHeader = m_blocks.back().block.GetHeader();
        auto pHeader = std::make_shared<BlockHeader>(
            (uint16_t)1,
            pPrevHeader->GetHeight() + 1,
            (TimeUtil::Now() - 1000) + pPrevHeader->GetHeight(),
            Hash(pPrevHeader->GetHash()),
            CalcHeaderRoot(),
            CalcOutputRoot(pTransaction->GetOutputs()),
            CalcRangeProofRoot(pTransaction->GetOutputs()),
            CalcKernelRoot(pTransaction->GetKernels()),
            Crypto::AddBlindingFactors({ pPrevHeader->GetOffset(), pTransaction->GetOffset() }, {}),
            CalcOutputSize(pTransaction->GetOutputs()),
            CalcKernelSize(pTransaction->GetKernels()),
            pPrevHeader->GetTotalDifficulty() + 1 + additionalDifficulty,
            10,
            ByteBuffer(pTransaction->GetHash().GetData()).ReadU64(),
            GeneratePoW(pPrevHeader, pTransaction->GetHash())
        );

        std::cout << "Mined Block: " << pHeader->GetHeight() << " - " << pHeader->Format() << std::endl;

        FullBlock block(pHeader, TransactionBody(pTransaction->GetBody()));
        MinedBlock minedBlock({
            block,
            Consensus::REWARD + block.GetTotalFees(),
            std::nullopt,
            txs
        });
        m_blocks.push_back(minedBlock);

        return minedBlock;
    }

    void Rewind(const size_t nextHeight)
    {
        m_blocks.erase(m_blocks.begin() + nextHeight, m_blocks.end());
    }

    Hash CalcHeaderRoot()
    {
        std::vector<BlockHeader> headers;

        std::transform(
            m_blocks.cbegin(), m_blocks.cend(),
            std::back_inserter(headers),
            [](const MinedBlock& block) { return *block.block.GetHeader(); }
        );

        return RootCalculator::CalculateRoot(headers);
    }

    Hash CalcKernelRoot(const std::vector<TransactionKernel>& additionalKernels = {})
    {
        return RootCalculator::CalculateRoot(GetAllKernels(additionalKernels));
    }

    uint64_t CalcKernelSize(const std::vector<TransactionKernel>& additionalKernels = {})
    {
        return LeafIndex::At(GetAllKernels(additionalKernels).size()).GetPosition();
    }

    Hash CalcOutputRoot(const std::vector<TransactionOutput>& additionalOutputs = {})
    {
        std::vector<TransactionOutput> allOutputs = GetAllOutputs(additionalOutputs);

        std::vector<OutputIdentifier> outputs;
        std::transform(
            allOutputs.cbegin(), allOutputs.cend(),
            std::back_inserter(outputs),
            [](const TransactionOutput& output) { return OutputIdentifier::FromOutput(output); }
        );

        return RootCalculator::CalculateRoot(outputs);
    }

    uint64_t CalcOutputSize(const std::vector<TransactionOutput>& additionalOutputs = {})
    {
        return LeafIndex::At(GetAllOutputs(additionalOutputs).size()).GetPosition();
    }

    Hash CalcRangeProofRoot(const std::vector<TransactionOutput>& additionalOutputs = {})
    {
        std::vector<TransactionOutput> allOutputs = GetAllOutputs(additionalOutputs);

        std::vector<RangeProof> rangeProofs;
        std::transform(
            allOutputs.cbegin(), allOutputs.cend(),
            std::back_inserter(rangeProofs),
            [](const TransactionOutput& output) { return output.GetRangeProof(); }
        );

        return RootCalculator::CalculateRoot(rangeProofs);
    }

    // To make this deterministic, we take in a "randomness", which typically consists of the offset
    ProofOfWork GeneratePoW(const BlockHeaderPtr& pPreviousHeader, const CBigInteger<32>& randomness)
    {
        ByteBuffer deserializer(randomness.GetData());

        std::vector<uint64_t> nonces = pPreviousHeader->GetProofOfWork().GetProofNonces();
        nonces[0] += deserializer.ReadU64();
        nonces[1] += deserializer.ReadU64();
        nonces[2] += deserializer.ReadU64();
        nonces[3] += deserializer.ReadU64();

        return ProofOfWork(
            pPreviousHeader->GetProofOfWork().GetEdgeBits(),
            std::move(nonces)
        );
    }

private:
    std::vector<TransactionKernel> GetAllKernels(const std::vector<TransactionKernel>& additionalKernels = {})
    {
        std::vector<TransactionKernel> kernels;
        for (const MinedBlock& block : m_blocks)
        {
            const auto& blockKernels = block.block.GetKernels();
            std::copy(blockKernels.cbegin(), blockKernels.cend(), std::back_inserter(kernels));
        }

        kernels.insert(kernels.end(), additionalKernels.cbegin(), additionalKernels.cend());
        return kernels;
    }

    std::vector<TransactionOutput> GetAllOutputs(const std::vector<TransactionOutput>& additionalOutputs = {})
    {
        std::vector<TransactionOutput> outputs;
        for (const MinedBlock& block : m_blocks)
        {
            const auto& blockOutputs = block.block.GetOutputs();
            std::copy(blockOutputs.cbegin(), blockOutputs.cend(), std::back_inserter(outputs));
        }

        std::copy(additionalOutputs.cbegin(), additionalOutputs.cend(), std::back_inserter(outputs));
        
        return outputs;
    }

    IBlockChain::Ptr m_pBlockChain;
    std::vector<MinedBlock> m_blocks;
};