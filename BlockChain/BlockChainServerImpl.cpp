#include "BlockChainServerImpl.h"
#include "BlockHydrator.h"
#include "Validators/BlockHeaderValidator.h"
#include "Validators/BlockValidator.h"
#include "Processors/BlockHeaderProcessor.h"
#include "Processors/TxHashSetProcessor.h"
#include "Processors/BlockProcessor.h"

#include <Config/Config.h>
#include <Crypto.h>
#include <PMMR/TxHashSet.h>

BlockChainServer::BlockChainServer(const Config& config, IDatabase& database)
	: m_config(config), m_database(database)
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
	m_pTxHashSetManager = new TxHashSetManager(m_config, m_database.GetBlockDB());
	m_pTransactionPool = TxPoolAPI::CreateTransactionPool(m_config, *m_pTxHashSetManager, m_database.GetBlockDB());
	m_pChainState = new ChainState(m_config, *m_pChainStore, *m_pBlockStore, *m_pHeaderMMR, *m_pTransactionPool, *m_pTxHashSetManager);
	m_pChainState->Initialize(genesisBlock.GetBlockHeader());

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

		TxPoolAPI::DestroyTransactionPool(m_pTransactionPool);
		m_pTransactionPool = nullptr;

		delete m_pTxHashSetManager;
		m_pTxHashSetManager = nullptr;
	}
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
	return BlockProcessor(m_config, *m_pChainState).ProcessBlock(block);
}

EBlockChainStatus BlockChainServer::AddCompactBlock(const CompactBlock& compactBlock)
{
	const CBigInteger<32>& hash = compactBlock.GetHash();

	std::unique_ptr<BlockHeader> pConfirmedHeader = m_pChainState->GetBlockHeaderByHeight(compactBlock.GetBlockHeader().GetHeight(), EChainType::CONFIRMED);
	if (pConfirmedHeader != nullptr && pConfirmedHeader->GetHash() == hash)
	{
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	std::unique_ptr<FullBlock> pHydratedBlock = BlockHydrator(*m_pChainState, *m_pTransactionPool).Hydrate(compactBlock);
	if (pHydratedBlock != nullptr)
	{
		return AddBlock(*pHydratedBlock);
	}

	return EBlockChainStatus::TRANSACTIONS_MISSING;
}

EBlockChainStatus BlockChainServer::ProcessTransactionHashSet(const Hash& blockHash, const std::string& path)
{
	const bool success = TxHashSetProcessor(m_config, *this, *m_pChainState, m_database.GetBlockDB()).ProcessTxHashSet(blockHash, path);

	return success ? EBlockChainStatus::SUCCESS : EBlockChainStatus::INVALID;
}

EBlockChainStatus BlockChainServer::AddTransaction(const Transaction& transaction, const EPoolType poolType)
{
	std::unique_ptr<BlockHeader> pLastConfimedHeader = m_pChainState->GetBlockHeaderByHeight(m_pChainState->GetHeight(EChainType::CONFIRMED), EChainType::CONFIRMED);
	if (pLastConfimedHeader != nullptr)
	{
		if (m_pTransactionPool->AddTransaction(transaction, poolType, *pLastConfimedHeader))
		{
			return EBlockChainStatus::SUCCESS;
		}
	}

	return EBlockChainStatus::INVALID;
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
		std::unique_ptr<BlockHeader> pHeader = m_pChainState->GetBlockHeaderByHash(hash);
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
	return m_pChainState->GetBlockHeaderByHash(hash);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByCommitment(const Hash& outputCommitment) const
{
	// TODO: Implement this
	return std::unique_ptr<BlockHeader>(nullptr);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetTipBlockHeader(const EChainType chainType) const
{
	return m_pChainState->GetBlockHeaderByHeight(m_pChainState->GetHeight(chainType), chainType);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHash(const CBigInteger<32>& hash) const
{
	return m_pChainState->GetBlockByHash(hash);
}


std::vector<std::pair<uint64_t, Hash>> BlockChainServer::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	return m_pChainState->GetBlocksNeeded(maxNumBlocks);
}

namespace BlockChainAPI
{
	EXPORT IBlockChainServer* StartBlockChainServer(const Config& config, IDatabase& database)
	{
		BlockChainServer* pServer = new BlockChainServer(config, database);
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
