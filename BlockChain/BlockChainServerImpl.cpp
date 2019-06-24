#include "BlockChainServerImpl.h"
#include "BlockHydrator.h"
#include "CompactBlockFactory.h"
#include "Validators/BlockHeaderValidator.h"
#include "Validators/BlockValidator.h"
#include "Processors/BlockHeaderProcessor.h"
#include "Processors/TxHashSetProcessor.h"
#include "Processors/BlockProcessor.h"
#include "ChainResyncer.h"

#include <Config/Config.h>
#include <Crypto/Crypto.h>
#include <PMMR/TxHashSet.h>
#include <Consensus/BlockTime.h>
#include <filesystem.h>
#include <algorithm>

BlockChainServer::BlockChainServer(const Config& config, IDatabase& database, TxHashSetManager& txHashSetManager, ITransactionPool& transactionPool)
	: m_config(config), m_database(database), m_txHashSetManager(txHashSetManager), m_transactionPool(transactionPool)
{

}

BlockChainServer::~BlockChainServer()
{
	Shutdown();
}

void BlockChainServer::Initialize()
{
	const FullBlock& genesisBlock = m_config.GetEnvironment().GetGenesisBlock();
	BlockIndex* pGenesisIndex = new BlockIndex(genesisBlock.GetHash(), 0, nullptr);

	m_pChainStore = new ChainStore(m_config, pGenesisIndex);
	m_pChainStore->Load();

	m_pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(m_config);

	m_pBlockStore = new BlockStore(m_config, m_database.GetBlockDB());
	m_pChainState = new ChainState(m_config, *m_pChainStore, *m_pBlockStore, *m_pHeaderMMR, m_transactionPool, m_txHashSetManager);
	m_pChainState->Initialize(genesisBlock.GetBlockHeader());

	// TODO: Trigger Compaction

	m_initialized = true;
}

void BlockChainServer::Shutdown()
{
	if (m_initialized)
	{
		m_initialized = false;

		m_pChainState->FlushAll();

		delete m_pChainState;
		m_pChainState = nullptr;

		delete m_pBlockStore;
		m_pBlockStore = nullptr;

		delete m_pChainStore;
		m_pChainStore = nullptr;

		HeaderMMRAPI::CloseHeaderMMR(m_pHeaderMMR);
		m_pHeaderMMR = nullptr;
	}
}

bool BlockChainServer::ResyncChain()
{
	return ChainResyncer(*m_pChainState).ResyncChain();
}

void BlockChainServer::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	m_pChainState->UpdateSyncStatus(syncStatus);
}

uint64_t BlockChainServer::GetHeight(const EChainType chainType) const
{
	return m_pChainState->GetHeight(chainType);
}

uint64_t BlockChainServer::GetTotalDifficulty(const EChainType chainType) const
{
	return m_pChainState->GetTotalDifficulty(chainType);
}

EBlockChainStatus BlockChainServer::AddBlock(const FullBlock& block)
{
	return BlockProcessor(m_config, m_pBlockStore->GetBlockDB(), *m_pChainState).ProcessBlock(block);
}

EBlockChainStatus BlockChainServer::AddCompactBlock(const CompactBlock& compactBlock)
{
	const Hash& hash = compactBlock.GetHash();

	std::unique_ptr<BlockHeader> pConfirmedHeader = m_pChainState->GetBlockHeaderByHeight(compactBlock.GetBlockHeader().GetHeight(), EChainType::CONFIRMED);
	if (pConfirmedHeader != nullptr && pConfirmedHeader->GetHash() == hash)
	{
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	std::unique_ptr<FullBlock> pHydratedBlock = BlockHydrator(*m_pChainState, m_transactionPool).Hydrate(compactBlock);
	if (pHydratedBlock != nullptr)
	{
		const EBlockChainStatus added = AddBlock(*pHydratedBlock);
		if (added == EBlockChainStatus::INVALID)
		{
			return EBlockChainStatus::TRANSACTIONS_MISSING;
		}

		return added;
	}

	return EBlockChainStatus::TRANSACTIONS_MISSING;
}

std::string BlockChainServer::SnapshotTxHashSet(const BlockHeader& blockHeader)
{
	// TODO: Check horizon.
	const std::string destination = fs::temp_directory_path().string() + "Snapshots/TxHashSet." + blockHeader.FormatHash() + ".zip";
	if (m_txHashSetManager.SaveSnapshot(blockHeader, destination))
	{
		return destination;
	}

	return "";
}

EBlockChainStatus BlockChainServer::ProcessTransactionHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus)
{
	const bool success = TxHashSetProcessor(m_config, *this, *m_pChainState, m_database.GetBlockDB()).ProcessTxHashSet(blockHash, path, syncStatus);

	return success ? EBlockChainStatus::SUCCESS : EBlockChainStatus::INVALID;
}

EBlockChainStatus BlockChainServer::AddTransaction(const Transaction& transaction, const EPoolType poolType)
{
	std::unique_ptr<BlockHeader> pLastConfimedHeader = m_pChainState->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pLastConfimedHeader != nullptr)
	{
		if (m_transactionPool.AddTransaction(transaction, poolType, *pLastConfimedHeader))
		{
			return EBlockChainStatus::SUCCESS;
		}
	}

	return EBlockChainStatus::INVALID;
}

std::unique_ptr<Transaction> BlockChainServer::GetTransactionByKernelHash(const Hash& kernelHash) const
{
	return m_transactionPool.FindTransactionByKernelHash(kernelHash);
}

EBlockChainStatus BlockChainServer::AddBlockHeader(const BlockHeader& blockHeader)
{
	return BlockHeaderProcessor(m_config, *m_pChainState).ProcessSingleHeader(blockHeader);
}

EBlockChainStatus BlockChainServer::AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders)
{
	return BlockHeaderProcessor(m_config, *m_pChainState).ProcessSyncHeaders(blockHeaders);
}

std::vector<BlockHeader> BlockChainServer::GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const
{
	std::vector<BlockHeader> headers;
	for (const CBigInteger<32>& hash : hashes)
	{
		std::unique_ptr<BlockHeader> pHeader = m_pBlockStore->GetBlockHeaderByHash(hash);
		if (pHeader != nullptr)
		{
			headers.push_back(*pHeader);
		}
	}

	return headers;
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	return m_pChainState->GetBlockHeaderByHeight(height, chainType);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByHash(const CBigInteger<32>& hash) const
{
	return m_pBlockStore->GetBlockHeaderByHash(hash);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	return m_pChainState->GetBlockHeaderByCommitment(outputCommitment);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetTipBlockHeader(const EChainType chainType) const
{
	return m_pChainState->GetTipBlockHeader(chainType);
}

std::unique_ptr<CompactBlock> BlockChainServer::GetCompactBlockByHash(const Hash& hash) const
{
	std::unique_ptr<FullBlock> pBlock = m_pBlockStore->GetBlockByHash(hash);
	if (pBlock != nullptr)
	{
		return std::make_unique<CompactBlock>(CompactBlockFactory::CreateCompactBlock(*pBlock));
	}

	return std::unique_ptr<CompactBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByCommitment(const Commitment& outputCommitment) const
{
	std::unique_ptr<BlockHeader> pHeader = m_pChainState->GetBlockHeaderByCommitment(outputCommitment);
	if (pHeader != nullptr)
	{
		return GetBlockByHash(pHeader->GetHash());
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHash(const Hash& hash) const
{
	return m_pBlockStore->GetBlockByHash(hash);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHeight(const uint64_t height) const
{
	return m_pChainState->GetBlockByHeight(height);
}

std::vector<BlockWithOutputs> BlockChainServer::GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const
{
	const uint64_t highestHeight = (std::min)(m_pChainState->GetHeight(EChainType::CONFIRMED), maxHeight);

	std::vector<BlockWithOutputs> blocksWithOutputs;
	blocksWithOutputs.reserve(highestHeight - startHeight + 1);

	uint64_t height = startHeight;
	while (height <= highestHeight)
	{
		std::unique_ptr<BlockWithOutputs> pBlockWithOutputs = m_pChainState->GetBlockWithOutputs(height);
		if (pBlockWithOutputs != nullptr)
		{
			blocksWithOutputs.push_back(*pBlockWithOutputs);
		}

		++height;
	}

	return blocksWithOutputs;
}

std::vector<std::pair<uint64_t, Hash>> BlockChainServer::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	return m_pChainState->GetBlocksNeeded(maxNumBlocks);
}

bool BlockChainServer::ProcessNextOrphanBlock()
{
	std::unique_ptr<BlockHeader> pNextHeader = m_pChainState->GetBlockHeaderByHeight(m_pChainState->GetHeight(EChainType::CONFIRMED) + 1, EChainType::CANDIDATE);
	if (pNextHeader == nullptr)
	{
		return false;
	}

	std::unique_ptr<FullBlock> pOrphanBlock = m_pChainState->GetOrphanBlock(pNextHeader->GetHeight(), pNextHeader->GetHash());
	if (pOrphanBlock == nullptr)
	{
		return false;
	}

	return BlockProcessor(m_config, m_pBlockStore->GetBlockDB(), *m_pChainState).ProcessBlock(*pOrphanBlock) == EBlockChainStatus::SUCCESS;
}

namespace BlockChainAPI
{
	EXPORT IBlockChainServer* StartBlockChainServer(const Config& config, IDatabase& database, TxHashSetManager& txHashSetManager, ITransactionPool& transactionPool)
	{
		BlockChainServer* pServer = new BlockChainServer(config, database, txHashSetManager, transactionPool);
		pServer->Initialize();

		return pServer;
	}

	EXPORT void ShutdownBlockChainServer(IBlockChainServer* pBlockChainServer)
	{
		BlockChainServer* pServer = (BlockChainServer*)pBlockChainServer;
		pServer->Shutdown();

		delete pServer;
		pServer = nullptr;
	}
}
