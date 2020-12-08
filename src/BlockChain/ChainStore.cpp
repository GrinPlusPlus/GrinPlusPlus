#include "ChainStore.h"
#include <Core/Exceptions/BlockChainException.h>
#include <Common/Util/FileUtil.h>
#include <Common/Logger.h>
#include <vector>
#include <map>

ChainStore::ChainStore(const Chain::Ptr& pConfirmedChain, const Chain::Ptr& pCandidateChain)
	: m_pConfirmedChain(pConfirmedChain), m_pCandidateChain(pCandidateChain)
{

}

std::shared_ptr<Locked<ChainStore>> ChainStore::Load(const Config& config, std::shared_ptr<BlockIndex> pGenesisIndex)
{
	LOG_TRACE("Loading Chain");
	std::shared_ptr<BlockIndexAllocator> pAllocator = std::make_shared<BlockIndexAllocator>();

	const auto& chainPath = config.GetNodeConfig().GetChainPath();
	std::shared_ptr<Chain> pConfirmedChain = Chain::Load(pAllocator, EChainType::CONFIRMED, chainPath / "confirmed.chain", pGenesisIndex);
	if (pConfirmedChain == nullptr)
	{
		LOG_INFO("Failed to load confirmed chain");
		throw std::exception();
	}

	pAllocator->AddChain(pConfirmedChain);
	std::shared_ptr<Chain> pCandidateChain = Chain::Load(pAllocator, EChainType::CANDIDATE, chainPath / "candidate.chain", pGenesisIndex);
	if (pCandidateChain == nullptr)
	{
		LOG_INFO("Failed to load candidate chain");
		throw std::exception();
	}

	pAllocator->AddChain(pCandidateChain);
	//std::shared_ptr<Chain> pSyncChain = Chain::Load(pAllocator, EChainType::SYNC, chainPath / "sync.chain", pGenesisIndex);
	//if (pSyncChain == nullptr)
	//{
	//	LOG_INFO("Failed to load sync chain");
	//	throw std::exception();
	//}

	//pAllocator->AddChain(pSyncChain);

	auto pChainStore = std::shared_ptr<ChainStore>(new ChainStore(pConfirmedChain, pCandidateChain));
	return std::make_shared<Locked<ChainStore>>(Locked<ChainStore>(pChainStore));
}

void ChainStore::Commit()
{
	//m_pSyncChain->Commit();
	m_pCandidateChain->Commit();
	m_pConfirmedChain->Commit();
}

void ChainStore::Rollback() noexcept
{
	//m_pSyncChain->Rollback();
	m_pCandidateChain->Rollback();
	m_pConfirmedChain->Rollback();
}

void ChainStore::OnInitWrite(const bool batch)
{
	//m_pSyncChain->OnInitWrite(batch);
	m_pCandidateChain->OnInitWrite(batch);
	m_pConfirmedChain->OnInitWrite(batch);
}

void ChainStore::OnEndWrite()
{
	//m_pSyncChain->OnEndWrite();
	m_pCandidateChain->OnEndWrite();
	m_pConfirmedChain->OnEndWrite();
}

std::shared_ptr<const BlockIndex> ChainStore::FindCommonIndex(const EChainType chainType1, const EChainType chainType2) const
{
	std::shared_ptr<const Chain> pChain1 = GetChain(chainType1);
	std::shared_ptr<const Chain> pChain2 = GetChain(chainType2);

	uint64_t height = (std::min)(pChain1->GetHeight(), pChain2->GetHeight());
	std::shared_ptr<const BlockIndex> pChain1Index = pChain1->GetByHeight(height);
	std::shared_ptr<const BlockIndex> pChain2Index = pChain2->GetByHeight(height);

	while (pChain1Index->GetHash() != pChain2Index->GetHash())
	{
		--height;
		pChain1Index = pChain1->GetByHeight(height);
		pChain2Index = pChain2->GetByHeight(height);
	}

	return pChain1Index;
}

void ChainStore::ReorgChain(const EChainType source, const EChainType destination)
{
	ReorgChain(source, destination, GetChain(source)->GetHeight());
}

void ChainStore::ReorgChain(const EChainType source, const EChainType destination, const uint64_t height)
{
	std::shared_ptr<Chain> pSourceChain = GetChain(source);
	std::shared_ptr<Chain> pDestinationChain = GetChain(destination);

	if (pSourceChain->GetHeight() < height)
	{
		throw BLOCK_CHAIN_EXCEPTION("Can't reorg beyond tip");
	}

	std::shared_ptr<const BlockIndex> pBlockIndex = FindCommonIndex(source, destination);
	if (pBlockIndex == nullptr)
	{
		throw BLOCK_CHAIN_EXCEPTION("No common header found.");
	}

	const uint64_t commonHeight = (std::min)(pBlockIndex->GetHeight(), height);
	pDestinationChain->Rewind(commonHeight);
	for (uint64_t i = commonHeight + 1; i <= height; i++)
	{
		pDestinationChain->AddBlock(pSourceChain->GetHash(i), i);
	}
}

void ChainStore::AddBlock(const EChainType source, const EChainType destination, const uint64_t height)
{
	std::shared_ptr<Chain> pSourceChain = GetChain(source);
	std::shared_ptr<Chain> pDestinationChain = GetChain(destination);

	if (pDestinationChain->GetTip()->GetHeight() + 1 == height)
	{
		if (pSourceChain->GetTip()->GetHeight() >= height)
		{
			pDestinationChain->AddBlock(pSourceChain->GetHash(height), height);
			return;
		}
	}

	throw BLOCK_CHAIN_EXCEPTION("Failed to add block");
}

std::shared_ptr<Chain> ChainStore::GetChain(const EChainType chainType)
{
	if (chainType == EChainType::CONFIRMED)
	{
		return m_pConfirmedChain;
	}
	else if (chainType == EChainType::CANDIDATE)
	{
		return m_pCandidateChain;
	}
	//else if (chainType == EChainType::SYNC)
	//{
	//	return m_pSyncChain;
	//}

	throw std::exception();
}

std::shared_ptr<const Chain> ChainStore::GetChain(const EChainType chainType) const
{
	if (chainType == EChainType::CONFIRMED)
	{
		return m_pConfirmedChain;
	}
	else if (chainType == EChainType::CANDIDATE)
	{
		return m_pCandidateChain;
	}
	//else if (chainType == EChainType::SYNC)
	//{
	//	return m_pSyncChain;
	//}

	throw std::exception();
}