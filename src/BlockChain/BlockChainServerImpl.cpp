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

BlockChainServer::BlockChainServer(
	const Config& config,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<TxHashSetManager> pTxHashSetManager,
	std::shared_ptr<ITransactionPool> pTransactionPool,
	std::shared_ptr<Locked<ChainState>> pChainState,
	std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR)
	: m_config(config),
	m_pDatabase(pDatabase),
	m_pTxHashSetManager(pTxHashSetManager),
	m_pTransactionPool(pTransactionPool),
	m_pChainState(pChainState),
	m_pHeaderMMR(pHeaderMMR)
{

}

BlockChainServer::~BlockChainServer()
{

}

std::shared_ptr<BlockChainServer> BlockChainServer::Create(
	const Config& config,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<TxHashSetManager> pTxHashSetManager,
	std::shared_ptr<ITransactionPool> pTransactionPool)
{
	const FullBlock& genesisBlock = config.GetEnvironment().GetGenesisBlock();
	BlockIndex* pGenesisIndex = new BlockIndex(genesisBlock.GetHash(), 0, nullptr);

	std::shared_ptr<Locked<ChainStore>> pChainStore = ChainStore::Load(config, pGenesisIndex);
	std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(config);

	std::shared_ptr<Locked<ChainState>> pChainState = ChainState::Create(
		config,
		pChainStore,
		pDatabase,
		pHeaderMMR,
		pTransactionPool,
		pTxHashSetManager,
		genesisBlock.GetBlockHeader()
	);

	// FUTURE: Trigger Compaction

	return std::make_shared<BlockChainServer>(BlockChainServer(
		config,
		pDatabase,
		pTxHashSetManager,
		pTransactionPool,
		pChainState,
		pHeaderMMR
	));
}

bool BlockChainServer::ResyncChain()
{
	return ChainResyncer(m_pChainState).ResyncChain();
}

void BlockChainServer::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	m_pChainState->Read()->UpdateSyncStatus(syncStatus);
}

uint64_t BlockChainServer::GetHeight(const EChainType chainType) const
{
	return m_pChainState->Read()->GetHeight(chainType);
}

uint64_t BlockChainServer::GetTotalDifficulty(const EChainType chainType) const
{
	return m_pChainState->Read()->GetTotalDifficulty(chainType);
}

EBlockChainStatus BlockChainServer::AddBlock(const FullBlock& block)
{
	return BlockProcessor(m_config, m_pChainState).ProcessBlock(block);
}

EBlockChainStatus BlockChainServer::AddCompactBlock(const CompactBlock& compactBlock)
{
	const Hash& hash = compactBlock.GetHash();

	std::unique_ptr<BlockHeader> pConfirmedHeader = m_pChainState->Read()->GetBlockHeaderByHeight(compactBlock.GetBlockHeader().GetHeight(), EChainType::CONFIRMED);
	if (pConfirmedHeader != nullptr && pConfirmedHeader->GetHash() == hash)
	{
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	std::unique_ptr<FullBlock> pHydratedBlock = BlockHydrator(m_pTransactionPool).Hydrate(compactBlock);
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
	auto pReader = m_pChainState->Read();
	const std::string destination = StringUtil::Format("%sSnapshots/TxHashSet.%s.zip", fs::temp_directory_path().string(), blockHeader.ShortHash());
	if (m_pTxHashSetManager->SaveSnapshot(pReader->GetBlockDB().GetShared(), blockHeader, destination))
	{
		return destination;
	}

	return "";
}

EBlockChainStatus BlockChainServer::ProcessTransactionHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus)
{
	const bool success = TxHashSetProcessor(m_config, *this, m_pChainState, m_pDatabase).ProcessTxHashSet(blockHash, path, syncStatus);

	return success ? EBlockChainStatus::SUCCESS : EBlockChainStatus::INVALID;
}

EBlockChainStatus BlockChainServer::AddTransaction(const Transaction& transaction, const EPoolType poolType)
{
	auto pReader = m_pChainState->Read();
	std::unique_ptr<BlockHeader> pLastConfimedHeader = m_pChainState->Read()->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pLastConfimedHeader != nullptr)
	{
		if (m_pTransactionPool->AddTransaction(pReader->GetBlockDB().GetShared(), transaction, poolType, *pLastConfimedHeader))
		{
			return EBlockChainStatus::SUCCESS;
		}
	}

	return EBlockChainStatus::INVALID;
}

std::unique_ptr<Transaction> BlockChainServer::GetTransactionByKernelHash(const Hash& kernelHash) const
{
	return m_pTransactionPool->FindTransactionByKernelHash(kernelHash);
}

EBlockChainStatus BlockChainServer::AddBlockHeader(const BlockHeader& blockHeader)
{
	return BlockHeaderProcessor(m_config, m_pChainState).ProcessSingleHeader(blockHeader);
}

EBlockChainStatus BlockChainServer::AddBlockHeaders(const std::vector<BlockHeader>& blockHeaders)
{
	return BlockHeaderProcessor(m_config, m_pChainState).ProcessSyncHeaders(blockHeaders);
}

std::vector<BlockHeader> BlockChainServer::GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const
{
	std::vector<BlockHeader> headers;
	for (const CBigInteger<32>& hash : hashes)
	{
		std::unique_ptr<BlockHeader> pHeader = m_pDatabase->Read()->GetBlockHeader(hash);
		if (pHeader != nullptr)
		{
			headers.push_back(*pHeader);
		}
	}

	return headers;
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	return m_pChainState->Read()->GetBlockHeaderByHeight(height, chainType);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByHash(const CBigInteger<32>& hash) const
{
	return m_pDatabase->Read()->GetBlockHeader(hash);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	return m_pChainState->Read()->GetBlockHeaderByCommitment(outputCommitment);
}

std::unique_ptr<BlockHeader> BlockChainServer::GetTipBlockHeader(const EChainType chainType) const
{
	return m_pChainState->Read()->GetTipBlockHeader(chainType);
}

std::unique_ptr<CompactBlock> BlockChainServer::GetCompactBlockByHash(const Hash& hash) const
{
	std::unique_ptr<FullBlock> pBlock = m_pDatabase->Read()->GetBlock(hash);
	if (pBlock != nullptr)
	{
		return std::make_unique<CompactBlock>(CompactBlockFactory::CreateCompactBlock(*pBlock));
	}

	return std::unique_ptr<CompactBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByCommitment(const Commitment& outputCommitment) const
{
	std::unique_ptr<BlockHeader> pHeader = m_pChainState->Read()->GetBlockHeaderByCommitment(outputCommitment);
	if (pHeader != nullptr)
	{
		return GetBlockByHash(pHeader->GetHash());
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHash(const Hash& hash) const
{
	return m_pDatabase->Read()->GetBlock(hash);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHeight(const uint64_t height) const
{
	return m_pChainState->Read()->GetBlockByHeight(height);
}

std::vector<BlockWithOutputs> BlockChainServer::GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const
{
	const uint64_t highestHeight = (std::min)(m_pChainState->Read()->GetHeight(EChainType::CONFIRMED), maxHeight);

	std::vector<BlockWithOutputs> blocksWithOutputs;
	blocksWithOutputs.reserve(highestHeight - startHeight + 1);

	uint64_t height = startHeight;
	while (height <= highestHeight)
	{
		std::unique_ptr<BlockWithOutputs> pBlockWithOutputs = m_pChainState->Read()->GetBlockWithOutputs(height);
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
	return m_pChainState->Read()->GetBlocksNeeded(maxNumBlocks);
}

bool BlockChainServer::ProcessNextOrphanBlock()
{
	const uint64_t nextHeight = m_pChainState->Read()->GetHeight(EChainType::CONFIRMED) + 1;
	std::unique_ptr<BlockHeader> pNextHeader = m_pChainState->Read()->GetBlockHeaderByHeight(nextHeight, EChainType::CANDIDATE);
	if (pNextHeader == nullptr)
	{
		return false;
	}

	std::unique_ptr<FullBlock> pOrphanBlock = m_pChainState->Read()->GetOrphanBlock(pNextHeader->GetHeight(), pNextHeader->GetHash());
	if (pOrphanBlock == nullptr)
	{
		return false;
	}

	return BlockProcessor(m_config, m_pChainState).ProcessBlock(*pOrphanBlock) == EBlockChainStatus::SUCCESS;
}

namespace BlockChainAPI
{
	EXPORT std::shared_ptr<IBlockChainServer> StartBlockChainServer(
		const Config& config,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		TxHashSetManagerPtr pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool)
	{
		return BlockChainServer::Create(config, pDatabase, pTxHashSetManager, pTransactionPool);
	}
}
