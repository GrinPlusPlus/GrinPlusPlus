#include "BlockChainServerImpl.h"
#include "BlockHydrator.h"
#include "CompactBlockFactory.h"
#include "Validators/BlockHeaderValidator.h"
#include "Validators/BlockValidator.h"
#include "Processors/BlockHeaderProcessor.h"
#include "Processors/TxHashSetProcessor.h"
#include "Processors/BlockProcessor.h"
#include "ChainResyncer.h"

#include <Infrastructure/Logger.h>
#include <Core/Exceptions/BadDataException.h>
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

std::shared_ptr<BlockChainServer> BlockChainServer::Create(
	const Config& config,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<TxHashSetManager> pTxHashSetManager,
	std::shared_ptr<ITransactionPool> pTransactionPool)
{
	const FullBlock& genesisBlock = config.GetEnvironment().GetGenesisBlock();
	std::shared_ptr<BlockIndex> pGenesisIndex = std::make_shared<BlockIndex>(genesisBlock.GetHash(), 0);

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

	// Trigger Compaction
	auto pTxHashSet = pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet != nullptr)
	{
		pTxHashSet->Write()->Compact();
	}

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
	try
	{
		return BlockProcessor(m_config, m_pChainState).ProcessBlock(block);
	}
	catch (std::exception&)
	{
		return EBlockChainStatus::INVALID;
	}
}

EBlockChainStatus BlockChainServer::AddCompactBlock(const CompactBlock& compactBlock)
{
	const Hash& hash = compactBlock.GetHash();
	const uint64_t height = compactBlock.GetHeight();

	{
		auto pReader = m_pChainState->Read();
		auto pConfirmed = pReader->GetChainStore()->GetConfirmedChain()->GetByHeight(height);
		if (pConfirmed != nullptr && pConfirmed->GetHash() == hash)
		{
			return EBlockChainStatus::ALREADY_EXISTS;
		}
	}

	try
	{
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
	}
	catch (std::exception& e)
	{
		LOG_WARNING_F("Exception thrown: {}", e.what());
	}

	return EBlockChainStatus::TRANSACTIONS_MISSING;
}

fs::path BlockChainServer::SnapshotTxHashSet(BlockHeaderPtr pBlockHeader)
{
	auto pReader = m_pChainState->Read();
	const uint64_t horizon = Consensus::GetHorizonHeight(pReader->GetHeight(EChainType::CONFIRMED));
	if (pBlockHeader->GetHeight() < horizon)
	{
		throw BAD_DATA_EXCEPTION("TxHashSet snapshot requested beyond horizon.");
	}

	const std::string destination = StringUtil::Format(
		"{}Snapshots/TxHashSet.{}.zip",
		fs::temp_directory_path().string(),
		pBlockHeader->ShortHash()
	);

	return m_pTxHashSetManager->SaveSnapshot(pReader->GetBlockDB().GetShared(), pBlockHeader);
}

EBlockChainStatus BlockChainServer::ProcessTransactionHashSet(const Hash& blockHash, const fs::path& path, SyncStatus& syncStatus)
{
	try
	{
		const bool success = TxHashSetProcessor(m_config, *this, m_pChainState).ProcessTxHashSet(blockHash, path, syncStatus);
		if (success)
		{
			return EBlockChainStatus::SUCCESS;
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to process TxHashSet: {}", e.what());
	}

	return EBlockChainStatus::INVALID;
}

EBlockChainStatus BlockChainServer::AddTransaction(TransactionPtr pTransaction, const EPoolType poolType)
{
	try
	{
		auto pReader = m_pChainState->Read();
		auto pLastConfimedHeader = pReader->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pLastConfimedHeader != nullptr)
		{
			const EAddTransactionStatus status = m_pTransactionPool->AddTransaction(
				pReader->GetBlockDB().GetShared(),
				pReader->GetTxHashSet().GetShared(),
				pTransaction,
				poolType,
				*pLastConfimedHeader
			);
			if (status == EAddTransactionStatus::ADDED)
			{
				return EBlockChainStatus::SUCCESS;
			}
			else if (status == EAddTransactionStatus::TX_INVALID)
			{
				return EBlockChainStatus::INVALID;
			}
			else
			{
				return EBlockChainStatus::UNKNOWN_ERROR;
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	return EBlockChainStatus::INVALID;
}

TransactionPtr BlockChainServer::GetTransactionByKernelHash(const Hash& kernelHash) const
{
	return m_pTransactionPool->FindTransactionByKernelHash(kernelHash);
}

EBlockChainStatus BlockChainServer::AddBlockHeader(BlockHeaderPtr pBlockHeader)
{
	try
	{
		return BlockHeaderProcessor(m_config, m_pChainState).ProcessSingleHeader(pBlockHeader);
	}
	catch (std::exception&)
	{
		return EBlockChainStatus::INVALID;
	}
}

EBlockChainStatus BlockChainServer::AddBlockHeaders(const std::vector<BlockHeaderPtr>& blockHeaders)
{
	try
	{
		return BlockHeaderProcessor(m_config, m_pChainState).ProcessSyncHeaders(blockHeaders);
	}
	catch (BadDataException&)
	{
		return EBlockChainStatus::INVALID;
	}
	catch (std::exception&)
	{
		return EBlockChainStatus::UNKNOWN_ERROR;
	}
}

std::vector<BlockHeaderPtr> BlockChainServer::GetBlockHeadersByHash(const std::vector<CBigInteger<32>>& hashes) const
{
	auto pReader = m_pChainState->Read();

	std::vector<BlockHeaderPtr> headers;
	for (const CBigInteger<32>& hash : hashes)
	{
		BlockHeaderPtr pHeader = pReader->GetBlockHeaderByHash(hash);
		if (pHeader != nullptr)
		{
			headers.push_back(pHeader);
		}
	}

	return headers;
}

BlockHeaderPtr BlockChainServer::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	return m_pChainState->Read()->GetBlockHeaderByHeight(height, chainType);
}

BlockHeaderPtr BlockChainServer::GetBlockHeaderByHash(const CBigInteger<32>& hash) const
{
	return m_pChainState->Read()->GetBlockHeaderByHash(hash);
}

BlockHeaderPtr BlockChainServer::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	return m_pChainState->Read()->GetBlockHeaderByCommitment(outputCommitment);
}

BlockHeaderPtr BlockChainServer::GetTipBlockHeader(const EChainType chainType) const
{
	return m_pChainState->Read()->GetTipBlockHeader(chainType);
}

std::unique_ptr<CompactBlock> BlockChainServer::GetCompactBlockByHash(const Hash& hash) const
{
	std::unique_ptr<FullBlock> pBlock = m_pChainState->Read()->GetBlockByHash(hash);
	if (pBlock != nullptr)
	{
		return std::make_unique<CompactBlock>(CompactBlockFactory::CreateCompactBlock(*pBlock));
	}

	return std::unique_ptr<CompactBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByCommitment(const Commitment& outputCommitment) const
{
	auto pHeader = m_pChainState->Read()->GetBlockHeaderByCommitment(outputCommitment);
	if (pHeader != nullptr)
	{
		return GetBlockByHash(pHeader->GetHash());
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHash(const Hash& hash) const
{
	return m_pChainState->Read()->GetBlockByHash(hash);
}

std::unique_ptr<FullBlock> BlockChainServer::GetBlockByHeight(const uint64_t height) const
{
	return m_pChainState->Read()->GetBlockByHeight(height);
}

std::vector<BlockWithOutputs> BlockChainServer::GetOutputsByHeight(const uint64_t startHeight, const uint64_t maxHeight) const
{
	auto pChainStateReader = m_pChainState->Read();
	const uint64_t highestHeight = (std::min)(pChainStateReader->GetHeight(EChainType::CONFIRMED), maxHeight);

	std::vector<BlockWithOutputs> blocksWithOutputs;
	blocksWithOutputs.reserve(highestHeight - startHeight + 1);

	uint64_t height = startHeight;
	while (height <= highestHeight)
	{
		std::unique_ptr<BlockWithOutputs> pBlockWithOutputs = pChainStateReader->GetBlockWithOutputs(height);
		if (pBlockWithOutputs != nullptr)
		{
			blocksWithOutputs.push_back(*pBlockWithOutputs);
		}

		++height;
	}

	return blocksWithOutputs;
}

bool BlockChainServer::HasBlock(const uint64_t height, const Hash& hash) const
{
	auto pChainStateReader = m_pChainState->Read();
	
	auto pIndex = pChainStateReader->GetChainStore()->GetConfirmedChain()->GetByHeight(height);

	return pIndex != nullptr && pIndex->GetHash() == hash;
}

std::vector<std::pair<uint64_t, Hash>> BlockChainServer::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	return m_pChainState->Read()->GetBlocksNeeded(maxNumBlocks);
}

bool BlockChainServer::ProcessNextOrphanBlock()
{
	BlockHeaderPtr pNextHeader = nullptr;
	std::shared_ptr<const FullBlock> pOrphanBlock = nullptr;

	{
		auto pReader = m_pChainState->Read();
		const uint64_t height = pReader->GetHeight(EChainType::CONFIRMED) + 1;
		pNextHeader = pReader->GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
		if (pNextHeader == nullptr)
		{
			return false;
		}

		pOrphanBlock = pReader->GetOrphanBlock(height, pNextHeader->GetHash());
		if (pOrphanBlock == nullptr)
		{
			return false;
		}
	}

	try
	{
		return BlockProcessor(m_config, m_pChainState).ProcessBlock(*pOrphanBlock) == EBlockChainStatus::SUCCESS;
	}
	catch (std::exception&)
	{
		m_pChainState->Write()->GetOrphanPool()->RemoveOrphan(pNextHeader->GetHeight(), pNextHeader->GetHash());
		return false;
	}
}

namespace BlockChainAPI
{
	BLOCK_CHAIN_API std::shared_ptr<IBlockChainServer> StartBlockChainServer(
		const Config& config,
		std::shared_ptr<Locked<IBlockDB>> pDatabase,
		TxHashSetManagerPtr pTxHashSetManager,
		std::shared_ptr<ITransactionPool> pTransactionPool)
	{
		return BlockChainServer::Create(config, pDatabase, pTxHashSetManager, pTransactionPool);
	}
}
