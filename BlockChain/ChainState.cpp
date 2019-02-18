#include "ChainState.h"

#include <Consensus/BlockTime.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>
#include <PMMR/TxHashSetManager.h>

ChainState::ChainState(const Config& config, ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR, ITransactionPool& transactionPool, TxHashSetManager& txHashSetManager)
	: m_config(config), m_chainStore(chainStore), m_blockStore(blockStore), m_headerMMR(headerMMR), m_transactionPool(transactionPool), m_txHashSetManager(txHashSetManager)
{

}

ChainState::~ChainState()
{

}

void ChainState::Initialize(const BlockHeader& genesisHeader)
{
	std::unique_lock<std::shared_mutex> writeLock(m_chainMutex);

	Chain& candidateChain = m_chainStore.GetCandidateChain();
	const uint64_t candidateHeight = candidateChain.GetTip()->GetHeight();
	if (candidateHeight == 0)
	{
		m_blockStore.AddHeader(genesisHeader);
		m_headerMMR.AddHeader(genesisHeader);
	}

	const BlockIndex* pConfirmedIndex = m_chainStore.GetConfirmedChain().GetTip();
	const std::unique_ptr<BlockHeader> pConfirmedHeader = m_blockStore.GetBlockHeaderByHash(pConfirmedIndex->GetHash());
	m_txHashSetManager.Open(*pConfirmedHeader);
}

void ChainState::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	const Hash& candidateHeadHash = GetHeadHash_Locked(EChainType::CANDIDATE);
	std::unique_ptr<BlockHeader> pCandidateHead = m_blockStore.GetBlockHeaderByHash(candidateHeadHash);
	if (pCandidateHead != nullptr)
	{
		syncStatus.UpdateHeaderStatus(pCandidateHead->GetHeight(), pCandidateHead->GetTotalDifficulty());
	}

	const Hash& confirmedHeadHash = GetHeadHash_Locked(EChainType::CONFIRMED);
	std::unique_ptr<BlockHeader> pConfirmedHead = m_blockStore.GetBlockHeaderByHash(confirmedHeadHash);
	if (pConfirmedHead != nullptr)
	{
		syncStatus.UpdateBlockStatus(pConfirmedHead->GetHeight(), pConfirmedHead->GetTotalDifficulty());
	}
}

uint64_t ChainState::GetHeight(const EChainType chainType) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::unique_ptr<BlockHeader> pHead = GetHead_Locked(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetHeight();
	}

	return 0;
}

uint64_t ChainState::GetTotalDifficulty(const EChainType chainType) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::unique_ptr<BlockHeader> pHead = GetHead_Locked(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetTotalDifficulty();
	}

	return 0;
}

std::unique_ptr<BlockHeader> ChainState::GetTipBlockHeader(const EChainType chainType) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	return GetHead_Locked(chainType);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByHash(const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	return m_blockStore.GetBlockHeaderByHash(hash);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	const Chain& chain = m_chainStore.GetChain(chainType);
	const BlockIndex* pBlockIndex = chain.GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return m_blockStore.GetBlockHeaderByHash(pBlockIndex->GetHash());
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::unique_ptr<BlockHeader> pHeader(nullptr);

	std::optional<OutputLocation> outputLocationOpt = m_blockStore.GetBlockDB().GetOutputPosition(outputCommitment);
	if (outputLocationOpt.has_value())
	{
		BlockIndex* pBlockIndex = m_chainStore.GetChain(EChainType::CONFIRMED).GetByHeight(outputLocationOpt.value().GetBlockHeight());
		if (pBlockIndex != nullptr)
		{
			return m_blockStore.GetBlockHeaderByHash(pBlockIndex->GetHash());
		}
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}

std::unique_ptr<FullBlock> ChainState::GetBlockByHash(const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	return m_blockStore.GetBlockByHash(hash);
}

std::unique_ptr<FullBlock> ChainState::GetBlockByHeight(const uint64_t height) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	BlockIndex* pBlockIndex = m_chainStore.GetChain(EChainType::CONFIRMED).GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return m_blockStore.GetBlockByHash(pBlockIndex->GetHash());
	}
	else
	{
		return std::unique_ptr<FullBlock>(nullptr);
	}
}

std::unique_ptr<FullBlock> ChainState::GetOrphanBlock(const uint64_t height, const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	return m_orphanPool.GetOrphanBlock(height, hash);
}

std::unique_ptr<BlockWithOutputs> ChainState::GetBlockWithOutputs(const uint64_t height) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	ITxHashSet* pTxHashSet = m_txHashSetManager.GetTxHashSet();
	if (pTxHashSet == nullptr)
	{
		return std::unique_ptr<BlockWithOutputs>(nullptr);
	}

	BlockIndex* pBlockIndex = m_chainStore.GetChain(EChainType::CONFIRMED).GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		std::unique_ptr<FullBlock> pBlock = m_blockStore.GetBlockByHash(pBlockIndex->GetHash());
		if (pBlock != nullptr)
		{
			std::vector<OutputDisplayInfo> outputsFound;
			outputsFound.reserve(pBlock->GetTransactionBody().GetOutputs().size());

			const std::vector<TransactionOutput>& outputs = pBlock->GetTransactionBody().GetOutputs();
			for (const TransactionOutput& output : outputs)
			{
				std::optional<OutputLocation> locationOpt = m_blockStore.GetBlockDB().GetOutputPosition(output.GetCommitment());
				if (locationOpt.has_value())
				{
					const bool spent = !pTxHashSet->IsUnspent(locationOpt.value());
					outputsFound.emplace_back(OutputDisplayInfo(spent, OutputIdentifier::FromOutput(output), locationOpt.value(), output.GetRangeProof()));
				}
			}

			return std::make_unique<BlockWithOutputs>(BlockWithOutputs(BlockIdentifier::FromHeader(pBlock->GetBlockHeader()), std::move(outputsFound)));
		}
	}

	return std::unique_ptr<BlockWithOutputs>(nullptr);
}

std::vector<std::pair<uint64_t, Hash>> ChainState::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::vector<std::pair<uint64_t, Hash>> blocksNeeded;
	blocksNeeded.reserve(maxNumBlocks);

	Chain& candidateChain = m_chainStore.GetCandidateChain();
	const uint64_t candidateHeight = candidateChain.GetTip()->GetHeight();

	uint64_t nextHeight = m_chainStore.FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight() + 1;
	while (nextHeight <= candidateHeight)
	{
		const BlockIndex* pIndex = candidateChain.GetByHeight(nextHeight);
		if (!m_orphanPool.IsOrphan(nextHeight, pIndex->GetHash()))
		{
			blocksNeeded.emplace_back(std::pair<uint64_t, Hash>(nextHeight, pIndex->GetHash()));

			if (blocksNeeded.size() == maxNumBlocks)
			{
				break;
			}
		}

		++nextHeight;
	}

	return blocksNeeded;
}

std::unique_ptr<BlockHeader> ChainState::GetHead_Locked(const EChainType chainType) const
{
	const Hash& headHash = GetHeadHash_Locked(chainType);

	return m_blockStore.GetBlockHeaderByHash(headHash);
}

const Hash& ChainState::GetHeadHash_Locked(const EChainType chainType) const
{
	return m_chainStore.GetChain(chainType).GetTip()->GetHash();
}

LockedChainState ChainState::GetLocked()
{
	return LockedChainState(m_chainMutex, m_chainStore, m_blockStore, m_headerMMR, m_orphanPool, m_transactionPool, m_txHashSetManager);
}

void ChainState::FlushAll()
{
	std::unique_lock<std::shared_mutex> writeLock(m_chainMutex);

	m_headerMMR.Commit();
	m_chainStore.Flush();
	m_txHashSetManager.Flush();
}