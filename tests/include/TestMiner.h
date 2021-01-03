#pragma once

#include "TestServer.h"
#include "TxBuilder.h"
#include <Models/MinedBlock.h>

#include <Consensus.h>
#include <Core/Config.h>
#include <BlockChain/BlockChain.h>
#include <PMMR/HeaderMMR.h>
#include <PMMR/TxHashSet.h>
#include <Database/BlockDb.h>
#include <Common/Util/TimeUtil.h>
#include <Crypto/CSPRNG.h>
#include <Wallet/Keychain/KeyChain.h>

class TestMiner
{
public:
	TestMiner(const TestServer::Ptr& pTestServer)
		: m_pTestServer(pTestServer)
	{
		m_proofNonces = pTestServer->GetGenesisHeader()->GetProofOfWork().GetProofNonces();
	}

	//
	// Mines a chain with the given number of blocks, containing only a coinbase transaction.
	// All coinbases will be generated using the provided KeyChain.
	//
	std::vector<MinedBlock> MineChain(const KeyChain& keyChain, const uint64_t chainLength)
	{
		assert(chainLength > 0);

		std::vector<MinedBlock> minedBlocks;
		minedBlocks.reserve(chainLength);
		
		// Add the genesis block
		minedBlocks.push_back({ m_pTestServer->GetGenesisBlock(), Consensus::REWARD, std::nullopt });

		TxBuilder txBuilder(keyChain);

		BlockHeaderPtr pPreviousHeader = m_pTestServer->GetGenesisHeader();
		for (uint32_t i = 1; i < chainLength; i++)
		{
			KeyChainPath keyChainPath({ 0, i });
			const uint64_t coinbaseAmount = Consensus::REWARD;
			Test::Tx tx = txBuilder.BuildCoinbaseTx(keyChainPath, coinbaseAmount);

			FullBlock block = MineNextBlock(pPreviousHeader, *tx.pTransaction, {});
			if (m_pTestServer->GetBlockChain()->AddBlock(block) != EBlockChainStatus::SUCCESS)
			{
				throw std::exception();
			}

			minedBlocks.push_back({
				block,
				coinbaseAmount,
				std::make_optional<KeyChainPath>(keyChainPath)
			});

			pPreviousHeader = block.GetHeader();
		}

		return minedBlocks;
	}

	//
	// Mines a single block consisting of the provided transaction.
	// First, temporarily applies the additional blocks provided, if any.
	// No changes to the chain state are committed.
	//
    FullBlock MineNextBlock(
		const BlockHeaderPtr& pPreviousHeader,
		const Transaction& transaction,
		const std::vector<FullBlock>& blocksToApply = {})
    {
		Hash headerRoot = CalculateHeaderRoot(pPreviousHeader, blocksToApply);
		auto roots = CalculateTxHashSetInfo(pPreviousHeader, transaction, blocksToApply);

		auto pPrevHeader = blocksToApply.empty() ? pPreviousHeader : blocksToApply.back().GetHeader();

		auto pHeader = std::make_shared<BlockHeader>(
			(uint16_t)1,
			pPrevHeader->GetHeight() + 1,
			(TimeUtil::Now() - 1000) + pPrevHeader->GetHeight(),
			Hash(pPrevHeader->GetHash()),
			Hash(headerRoot),
			Hash(roots.GetOutputInfo().root),
			Hash(roots.GetRangeProofInfo().root),
			Hash(roots.GetKernelInfo().root),
			CalculateOffset(pPrevHeader, transaction),
			roots.GetOutputInfo().size,
			roots.GetKernelInfo().size,
			pPrevHeader->GetTotalDifficulty() + 1,
			10,
			CSPRNG::GenerateRandom(0, UINT64_MAX),
			GeneratePoW(pPrevHeader)
		);

		std::cout << "Mined Block: " << pHeader->GetHeight() << " - " << pHeader->Format()  << std::endl;

		return FullBlock(pHeader, TransactionBody(transaction.GetBody()));
    }

private:
	TestServer::Ptr m_pTestServer;
	std::vector<uint64_t> m_proofNonces;

	Hash CalculateHeaderRoot(
		const BlockHeaderPtr& pPreviousHeader,
		const std::vector<FullBlock>& blocksToApply)
	{
		auto pHeaderMMRBatch = m_pTestServer->GetHeaderMMR()->BatchWrite();
		pHeaderMMRBatch->Rewind(pPreviousHeader->GetHeight() + 1);

		for (const FullBlock& blockToApply : blocksToApply)
		{
			pHeaderMMRBatch->AddHeader(*blockToApply.GetHeader());
		}

		Hash headerRoot = pHeaderMMRBatch->Root(pPreviousHeader->GetHeight() + blocksToApply.size());

		pHeaderMMRBatch->Rollback();

		return headerRoot;
	}

	TxHashSetRoots CalculateTxHashSetInfo(
		const BlockHeaderPtr& pPreviousHeader,
		const Transaction& transaction,
		const std::vector<FullBlock>& blocksToApply)
	{
		auto pTxHashSetManager = m_pTestServer->GetTxHashSetManager()->BatchWrite();
		auto pBlockDB = m_pTestServer->GetDatabase()->GetBlockDB()->BatchWrite();

		pTxHashSetManager->GetTxHashSet()->Rewind(pBlockDB.GetShared(), *pPreviousHeader);

		for (const FullBlock& blockToApply : blocksToApply)
		{
			pTxHashSetManager->GetTxHashSet()->ApplyBlock(
				pBlockDB.GetShared(),
				blockToApply
			);
		}

		auto roots = pTxHashSetManager->GetTxHashSet()->GetRoots(
			pBlockDB.GetShared(),
			transaction.GetBody()
		);

		pBlockDB->Rollback();
		pTxHashSetManager->Rollback();

		return roots;
	}

	ProofOfWork GeneratePoW(const BlockHeaderPtr& pPreviousHeader)
	{
		m_proofNonces[0]++;

		return ProofOfWork(
			pPreviousHeader->GetProofOfWork().GetEdgeBits(),
			std::vector<uint64_t>(m_proofNonces.cbegin(), m_proofNonces.cend())
		);
	}

	BlindingFactor CalculateOffset(const BlockHeaderPtr& pPreviousHeader, const Transaction& transaction)
	{
		std::vector<BlindingFactor> offsets({
			pPreviousHeader->GetTotalKernelOffset(),
			transaction.GetOffset()
		});
		return Crypto::AddBlindingFactors(offsets, {});
	}
};