#include "ChainState.h"

#include <Consensus/BlockTime.h>
#include <Database/BlockDb.h>
#include <TxHashSet.h>
#include <TxPool/TransactionPool.h>

ChainState::ChainState(const Config& config, ChainStore& chainStore, BlockStore& blockStore, IHeaderMMR& headerMMR, ITransactionPool& transactionPool)
	: m_config(config), m_chainStore(chainStore), m_blockStore(blockStore), m_headerMMR(headerMMR), m_transactionPool(transactionPool), m_pTxHashSet(nullptr)
{

}

ChainState::~ChainState()
{

}

void ChainState::Initialize(const BlockHeader& genesisHeader)
{
	Chain& candidateChain = m_chainStore.GetCandidateChain();
	const uint64_t candidateHeight = candidateChain.GetTip()->GetHeight();
	if (candidateHeight == 0)
	{
		m_blockStore.AddHeader(genesisHeader);
		m_headerMMR.AddHeader(genesisHeader);
	}
	else
	{
		const uint64_t horizon = std::max(candidateHeight, (uint64_t)Consensus::CUT_THROUGH_HORIZON) - (uint64_t)Consensus::CUT_THROUGH_HORIZON;

		std::vector<Hash> hashesToLoad;

		for (size_t i = horizon; i <= candidateHeight; i++)
		{
			const Hash& hash = candidateChain.GetByHeight(i)->GetHash();
			hashesToLoad.emplace_back(hash);
		}

		m_blockStore.LoadHeaders(hashesToLoad);
	}

	m_pTxHashSet.reset(TxHashSetAPI::Open(m_config, m_blockStore.GetBlockDB()));
}

uint64_t ChainState::GetHeight(const EChainType chainType)
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::unique_ptr<BlockHeader> pHead = GetHead_Locked(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetHeight();
	}

	return 0;
}

uint64_t ChainState::GetTotalDifficulty(const EChainType chainType)
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::unique_ptr<BlockHeader> pHead = GetHead_Locked(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetProofOfWork().GetTotalDifficulty();
	}

	return 0;
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByHash(const Hash& hash)
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	return m_blockStore.GetBlockHeaderByHash(hash);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType)
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	Chain& chain = m_chainStore.GetChain(chainType);
	const BlockIndex* pBlockIndex = chain.GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return m_blockStore.GetBlockHeaderByHash(pBlockIndex->GetHash());
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}

std::unique_ptr<FullBlock> ChainState::GetOrphanBlock(const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	return m_orphanPool.GetOrphanBlock(hash);
}

std::vector<std::pair<uint64_t, Hash>> ChainState::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	std::shared_lock<std::shared_mutex> readLock(m_chainMutex);

	std::vector<std::pair<uint64_t, Hash>> blocksNeeded;
	blocksNeeded.reserve(maxNumBlocks);

	Chain& candidateChain = m_chainStore.GetCandidateChain();
	const uint64_t candidateHeight = candidateChain.GetTip()->GetHeight();

	uint64_t nextHeight = m_chainStore.GetConfirmedChain().GetTip()->GetHeight() + 1;
	while (nextHeight <= candidateHeight)
	{
		const BlockIndex* pIndex = candidateChain.GetByHeight(nextHeight);
		blocksNeeded.emplace_back(std::pair<uint64_t, Hash>(nextHeight++, pIndex->GetHash()));

		if (blocksNeeded.size() == maxNumBlocks)
		{
			break;
		}
	}

	return blocksNeeded;
}

std::unique_ptr<BlockHeader> ChainState::GetHead_Locked(const EChainType chainType)
{
	const Hash& headHash = GetHeadHash_Locked(chainType);

	return m_blockStore.GetBlockHeaderByHash(headHash);
}

const Hash& ChainState::GetHeadHash_Locked(const EChainType chainType)
{
	return m_chainStore.GetChain(chainType).GetTip()->GetHash();
}

LockedChainState ChainState::GetLocked()
{
	return LockedChainState(m_chainMutex, m_chainStore, m_blockStore, m_headerMMR, m_orphanPool, m_transactionPool, m_pTxHashSet);
}

void ChainState::FlushAll()
{
	std::unique_lock<std::shared_mutex> writeLock(m_chainMutex);

	m_headerMMR.Commit();
	m_chainStore.Flush();
	if (m_pTxHashSet != nullptr)
	{
		m_pTxHashSet->Commit();
	}
}