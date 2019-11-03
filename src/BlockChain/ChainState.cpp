#include "ChainState.h"

#include <Consensus/BlockTime.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>
#include <PMMR/TxHashSetManager.h>

ChainState::ChainState(
	const Config& config,
	std::shared_ptr<Locked<ChainStore>> pChainStore,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR,
	std::shared_ptr<ITransactionPool> pTransactionPool,
	std::shared_ptr<TxHashSetManager> pTxHashSetManager)
	: m_config(config),
	m_pChainStore(pChainStore),
	m_pBlockDB(pDatabase),
	m_pHeaderMMR(pHeaderMMR),
	m_pTransactionPool(pTransactionPool),
	m_pTxHashSetManager(pTxHashSetManager),
	m_pOrphanPool(std::shared_ptr<OrphanPool>(new OrphanPool()))
{

}

std::shared_ptr<Locked<ChainState>> ChainState::Create(
	const Config& config,
	std::shared_ptr<Locked<ChainStore>> pChainStore,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR,
	std::shared_ptr<ITransactionPool> pTransactionPool,
	std::shared_ptr<TxHashSetManager> pTxHashSetManager,
	const BlockHeader& genesisHeader)
{
	std::shared_ptr<const Chain> pCandidateChain = pChainStore->Read()->GetCandidateChain();
	const uint64_t candidateHeight = pCandidateChain->GetTip()->GetHeight();
	if (candidateHeight == 0)
	{
		pDatabase->Write()->AddBlockHeader(genesisHeader);
		pHeaderMMR->Write()->AddHeader(genesisHeader);
	}

	const BlockIndex* pConfirmedIndex = pChainStore->Read()->GetConfirmedChain()->GetTip();
	const std::unique_ptr<BlockHeader> pConfirmedHeader = pDatabase->Read()->GetBlockHeader(pConfirmedIndex->GetHash());
	pTxHashSetManager->Open(*pConfirmedHeader);

	std::shared_ptr<ChainState> pChainState(new ChainState(config, pChainStore, pDatabase, pHeaderMMR, pTransactionPool, pTxHashSetManager));
	return std::make_shared<Locked<ChainState>>(Locked<ChainState>(pChainState));
}

void ChainState::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	const Hash& candidateHeadHash = GetChainStore()->GetChain(EChainType::CANDIDATE)->GetTip()->GetHash();
	std::unique_ptr<BlockHeader> pCandidateHead = m_pBlockDB->Read()->GetBlockHeader(candidateHeadHash);
	if (pCandidateHead != nullptr)
	{
		syncStatus.UpdateHeaderStatus(pCandidateHead->GetHeight(), pCandidateHead->GetTotalDifficulty());
	}

	const Hash& confirmedHeadHash = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetTip()->GetHash();
	std::unique_ptr<BlockHeader> pConfirmedHead = m_pBlockDB->Read()->GetBlockHeader(confirmedHeadHash);
	if (pConfirmedHead != nullptr)
	{
		syncStatus.UpdateBlockStatus(pConfirmedHead->GetHeight(), pConfirmedHead->GetTotalDifficulty());
	}
}

uint64_t ChainState::GetHeight(const EChainType chainType) const
{
	std::unique_ptr<BlockHeader> pHead = GetTipBlockHeader(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetHeight();
	}

	return 0;
}

uint64_t ChainState::GetTotalDifficulty(const EChainType chainType) const
{
	std::unique_ptr<BlockHeader> pHead = GetTipBlockHeader(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetTotalDifficulty();
	}

	return 0;
}

std::unique_ptr<BlockHeader> ChainState::GetTipBlockHeader(const EChainType chainType) const
{
	const Hash& headHash = GetChainStore()->GetChain(chainType)->GetTip()->GetHash();

	return m_pBlockDB->Read()->GetBlockHeader(headHash);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByHash(const Hash& hash) const
{
	return m_pBlockDB->Read()->GetBlockHeader(hash);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	std::shared_ptr<const Chain> pChain = GetChainStore()->GetChain(chainType);
	const BlockIndex* pBlockIndex = pChain->GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return m_pBlockDB->Read()->GetBlockHeader(pBlockIndex->GetHash());
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}

std::unique_ptr<BlockHeader> ChainState::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	std::unique_ptr<BlockHeader> pHeader(nullptr);

	std::unique_ptr<OutputLocation> pOutputLocation = m_pBlockDB->Read()->GetOutputPosition(outputCommitment);
	if (pOutputLocation != nullptr)
	{
		const BlockIndex* pBlockIndex = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetByHeight(pOutputLocation->GetBlockHeight());
		if (pBlockIndex != nullptr)
		{
			return m_pBlockDB->Read()->GetBlockHeader(pBlockIndex->GetHash());
		}
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}

std::unique_ptr<FullBlock> ChainState::GetBlockByHash(const Hash& hash) const
{
	return m_pBlockDB->Read()->GetBlock(hash);
}

std::unique_ptr<FullBlock> ChainState::GetBlockByHeight(const uint64_t height) const
{
	const BlockIndex* pBlockIndex = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return m_pBlockDB->Read()->GetBlock(pBlockIndex->GetHash());
	}
	else
	{
		return std::unique_ptr<FullBlock>(nullptr);
	}
}

std::unique_ptr<FullBlock> ChainState::GetOrphanBlock(const uint64_t height, const Hash& hash) const
{
	return m_pOrphanPool->GetOrphanBlock(height, hash);
}

std::unique_ptr<BlockWithOutputs> ChainState::GetBlockWithOutputs(const uint64_t height) const
{
	ITxHashSetPtr pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet == nullptr)
	{
		return std::unique_ptr<BlockWithOutputs>(nullptr);
	}

	const BlockIndex* pBlockIndex = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		std::unique_ptr<FullBlock> pBlock = m_pBlockDB->Read()->GetBlock(pBlockIndex->GetHash());
		if (pBlock != nullptr)
		{
			std::vector<OutputDisplayInfo> outputsFound;
			outputsFound.reserve(pBlock->GetTransactionBody().GetOutputs().size());

			const std::vector<TransactionOutput>& outputs = pBlock->GetTransactionBody().GetOutputs();
			for (const TransactionOutput& output : outputs)
			{
				std::unique_ptr<OutputLocation> pOutputLocation = m_pBlockDB->Read()->GetOutputPosition(output.GetCommitment());
				if (pOutputLocation != nullptr)
				{
					const bool spent = !pTxHashSet->IsUnspent(*pOutputLocation);
					outputsFound.emplace_back(OutputDisplayInfo(spent, OutputIdentifier::FromOutput(output), *pOutputLocation, output.GetRangeProof()));
				}
			}

			return std::make_unique<BlockWithOutputs>(BlockWithOutputs(BlockIdentifier::FromHeader(pBlock->GetBlockHeader()), std::move(outputsFound)));
		}
	}

	return std::unique_ptr<BlockWithOutputs>(nullptr);
}

std::vector<std::pair<uint64_t, Hash>> ChainState::GetBlocksNeeded(const uint64_t maxNumBlocks) const
{
	std::vector<std::pair<uint64_t, Hash>> blocksNeeded;
	blocksNeeded.reserve(maxNumBlocks);

	std::shared_ptr<const Chain> pCandidateChain = GetChainStore()->GetCandidateChain();
	const uint64_t candidateHeight = pCandidateChain->GetTip()->GetHeight();

	uint64_t nextHeight = GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED)->GetHeight() + 1;
	while (nextHeight <= candidateHeight)
	{
		const BlockIndex* pIndex = pCandidateChain->GetByHeight(nextHeight);
		if (!m_pOrphanPool->IsOrphan(nextHeight, pIndex->GetHash()))
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

//LockedChainState ChainState::GetLocked()
//{
//	return LockedChainState(
//		m_chainMutex,
//		m_pChainStore,
//		m_pBlockDB,
//		m_pHeaderMMR,
//		m_orphanPool,
//		m_pTransactionPool,
//		m_pTxHashSetManager
//	);
//}

void ChainState::Commit()
{
	if (!m_chainStoreWriter.IsNull())
	{
		m_chainStoreWriter->Commit();
	}

	if (!m_blockDBWriter.IsNull())
	{
		m_blockDBWriter->Commit();
	}

	if (!m_headerMMRWriter.IsNull())
	{
		m_headerMMRWriter->Commit();
	}

	m_pTxHashSetManager->Flush();
}

void ChainState::Rollback()
{
	if (!m_chainStoreWriter.IsNull())
	{
		m_chainStoreWriter->Rollback();
	}

	if (!m_blockDBWriter.IsNull())
	{
		m_blockDBWriter->Rollback();
	}

	if (!m_headerMMRWriter.IsNull())
	{
		m_headerMMRWriter->Rollback();
	}
}

void ChainState::OnInitWrite()
{
	m_chainStoreWriter.Clear();
	m_blockDBWriter.Clear();
	m_headerMMRWriter.Clear();
}

void ChainState::OnEndWrite()
{
	m_chainStoreWriter.Clear();
	m_blockDBWriter.Clear();
	m_headerMMRWriter.Clear();
}

//void ChainState::FlushAll()
//{
//	// TODO: m_pHeaderMMR->Commit();
//	GetChainStore()->Flush();
//	m_pTxHashSetManager->Flush();
//}