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
	std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager)
	: m_config(config),
	m_pChainStore(pChainStore),
	m_pBlockDB(pDatabase),
	m_pHeaderMMR(pHeaderMMR),
	m_pTransactionPool(pTransactionPool),
	m_pTxHashSetManager(pTxHashSetManager),
	m_pOrphanPool(std::make_shared<OrphanPool>())
{

}

std::shared_ptr<Locked<ChainState>> ChainState::Create(
	const Config& config,
	std::shared_ptr<Locked<ChainStore>> pChainStore,
	std::shared_ptr<Locked<IBlockDB>> pDatabase,
	std::shared_ptr<Locked<IHeaderMMR>> pHeaderMMR,
	std::shared_ptr<ITransactionPool> pTransactionPool,
	std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
	const FullBlock& genesisBlock)
{
	std::shared_ptr<const Chain> pCandidateChain = pChainStore->Read()->GetCandidateChain();
	const uint64_t candidateHeight = pCandidateChain->GetTip()->GetHeight();
	if (candidateHeight == 0)
	{
		auto locked = MultiLocker().Lock(*pDatabase, *pHeaderMMR);
		std::get<0>(locked)->AddBlockHeader(genesisBlock.GetHeader());
		std::get<1>(locked)->AddHeader(*genesisBlock.GetHeader());
		pTxHashSetManager->Write()->Open(genesisBlock.GetHeader(), genesisBlock);

		const BlockSums blockSums(
			genesisBlock.GetOutputs().front().GetCommitment(),
			genesisBlock.GetKernels().front().GetCommitment()
		);
		std::get<0>(locked)->AddBlockSums(genesisBlock.GetHash(), blockSums);
	}

	auto pConfirmedIndex = pChainStore->Read()->GetConfirmedChain()->GetTip();
	auto pConfirmedHeader = pDatabase->Read()->GetBlockHeader(pConfirmedIndex->GetHash());
	pTxHashSetManager->Write()->Open(pConfirmedHeader, genesisBlock);

	std::shared_ptr<ChainState> pChainState(new ChainState(config, pChainStore, pDatabase, pHeaderMMR, pTransactionPool, pTxHashSetManager));
	return std::make_shared<Locked<ChainState>>(Locked<ChainState>(pChainState));
}

void ChainState::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	const Hash& candidateHeadHash = GetChainStore()->GetChain(EChainType::CANDIDATE)->GetTipHash();
	auto pCandidateHead = GetBlockDB()->GetBlockHeader(candidateHeadHash);
	if (pCandidateHead != nullptr)
	{
		syncStatus.UpdateHeaderStatus(pCandidateHead->GetHeight(), pCandidateHead->GetTotalDifficulty());
	}

	const Hash& confirmedHeadHash = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetTipHash();
	auto pConfirmedHead = GetBlockDB()->GetBlockHeader(confirmedHeadHash);
	if (pConfirmedHead != nullptr)
	{
		syncStatus.UpdateBlockStatus(
			pConfirmedHead->GetHeight(),
			pConfirmedHead->GetTotalDifficulty(),
			pConfirmedHead->GetTimestamp()
		);
	}
}

uint64_t ChainState::GetHeight(const EChainType chainType) const
{
	return GetChainStore()->GetChain(chainType)->GetHeight();
}

uint64_t ChainState::GetTotalDifficulty(const EChainType chainType) const
{
	auto pHead = GetTipBlockHeader(chainType);
	if (pHead != nullptr)
	{
		return pHead->GetTotalDifficulty();
	}

	return 0;
}

BlockHeaderPtr ChainState::GetTipBlockHeader(const EChainType chainType) const
{
	const Hash& headHash = GetChainStore()->GetChain(chainType)->GetTipHash();

	return GetBlockDB()->GetBlockHeader(headHash);
}

BlockHeaderPtr ChainState::GetBlockHeaderByHash(const Hash& hash) const
{
	return GetBlockDB()->GetBlockHeader(hash);
}

BlockHeaderPtr ChainState::GetBlockHeaderByHeight(const uint64_t height, const EChainType chainType) const
{
	auto pBlockIndex = GetChainStore()->GetChain(chainType)->GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return GetBlockDB()->GetBlockHeader(pBlockIndex->GetHash());
	}

	return BlockHeaderPtr(nullptr);
}

BlockHeaderPtr ChainState::GetBlockHeaderByCommitment(const Commitment& outputCommitment) const
{
	BlockHeaderPtr pHeader(nullptr);

	std::unique_ptr<OutputLocation> pOutputLocation = GetBlockDB()->GetOutputPosition(outputCommitment);
	if (pOutputLocation != nullptr)
	{
		auto pBlockIndex = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetByHeight(pOutputLocation->GetBlockHeight());
		if (pBlockIndex != nullptr)
		{
			return GetBlockDB()->GetBlockHeader(pBlockIndex->GetHash());
		}
	}

	return BlockHeaderPtr(nullptr);
}

std::unique_ptr<FullBlock> ChainState::GetBlockByHash(const Hash& hash) const
{
	return GetBlockDB()->GetBlock(hash);
}

std::unique_ptr<FullBlock> ChainState::GetBlockByHeight(const uint64_t height) const
{
	auto pBlockIndex = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		return GetBlockDB()->GetBlock(pBlockIndex->GetHash());
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::shared_ptr<const FullBlock> ChainState::GetOrphanBlock(const uint64_t height, const Hash& hash) const
{
	return m_pOrphanPool->GetOrphanBlock(height, hash);
}

std::unique_ptr<BlockWithOutputs> ChainState::GetBlockWithOutputs(const uint64_t height) const
{
	auto pBlockIndex = GetChainStore()->GetChain(EChainType::CONFIRMED)->GetByHeight(height);
	if (pBlockIndex != nullptr)
	{
		std::unique_ptr<FullBlock> pBlock = GetBlockDB()->GetBlock(pBlockIndex->GetHash());
		if (pBlock != nullptr)
		{
			std::vector<OutputDTO> outputsFound;
			outputsFound.reserve(pBlock->GetTransactionBody().GetOutputs().size());

			const std::vector<TransactionOutput>& outputs = pBlock->GetTransactionBody().GetOutputs();
			for (const TransactionOutput& output : outputs)
			{
				std::unique_ptr<OutputLocation> pOutputLocation = GetBlockDB()->GetOutputPosition(output.GetCommitment());
				if (pOutputLocation != nullptr)
				{
					outputsFound.emplace_back(OutputDTO{ false, OutputIdentifier::FromOutput(output), *pOutputLocation, output.GetRangeProof() });
				}
			}

			return std::make_unique<BlockWithOutputs>(BlockWithOutputs{ BlockIdentifier::FromHeader(*pBlock->GetHeader()), std::move(outputsFound) });
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
		auto pIndex = pCandidateChain->GetByHeight(nextHeight);
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

	if (!m_txHashSetWriter.IsNull())
	{
		m_txHashSetWriter->Commit();
	}
}

void ChainState::Rollback() noexcept
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

	if (!m_txHashSetWriter.IsNull())
	{
		m_txHashSetWriter->Rollback();
	}
}

void ChainState::OnInitWrite(const bool /*batch*/)
{
	m_chainStoreWriter.Clear();
	m_blockDBWriter.Clear();
	m_headerMMRWriter.Clear();
	m_txHashSetWriter.Clear();

	auto locked = MultiLocker().BatchLock(*m_pChainStore, *m_pBlockDB, *m_pHeaderMMR, *m_pTxHashSetManager);
	m_chainStoreWriter = std::get<0>(locked);
	m_blockDBWriter = std::get<1>(locked);
	m_headerMMRWriter = std::get<2>(locked);
	m_txHashSetWriter = std::get<3>(locked);
}

void ChainState::OnEndWrite()
{
	m_chainStoreWriter.Clear();
	m_blockDBWriter.Clear();
	m_headerMMRWriter.Clear();
	m_txHashSetWriter.Clear();
}