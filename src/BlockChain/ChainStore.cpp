#include "ChainStore.h"
#include <Common/Util/FileUtil.h>
#include <Infrastructure/Logger.h>
#include <vector>
#include <map>

ChainStore::ChainStore(std::shared_ptr<Chain> pConfirmedChain, std::shared_ptr<Chain> pCandidateChain, std::shared_ptr<Chain> pSyncChain)
	: m_pConfirmedChain(pConfirmedChain), m_pCandidateChain(pCandidateChain), m_pSyncChain(pSyncChain)
{

}

std::shared_ptr<Locked<ChainStore>> ChainStore::Load(const Config& config, BlockIndex* pGenesisIndex)
{
	LOG_TRACE("Loading Chain");
	BlockIndexAllocator allocator(std::vector<std::shared_ptr<Chain>>({ }));

	std::shared_ptr<Chain> pConfirmedChain = Chain::Load(allocator, EChainType::CONFIRMED, config.GetChainDirectory() + "confirmed.chain", pGenesisIndex);
	if (pConfirmedChain == nullptr)
	{
		LOG_INFO("Failed to load confirmed chain");
		throw std::exception();
	}

	allocator.AddChain(pConfirmedChain);
	std::shared_ptr<Chain> pCandidateChain = Chain::Load(allocator, EChainType::CANDIDATE, config.GetChainDirectory() + "candidate.chain", pGenesisIndex);
	if (pCandidateChain == nullptr)
	{
		LOG_INFO("Failed to load candidate chain");
		throw std::exception();
	}

	allocator.AddChain(pCandidateChain);
	std::shared_ptr<Chain> pSyncChain = Chain::Load(allocator, EChainType::SYNC, config.GetChainDirectory() + "sync.chain", pGenesisIndex);
	if (pSyncChain == nullptr)
	{
		LOG_INFO("Failed to load sync chain");
		throw std::exception();
	}

	auto pChainStore = std::shared_ptr<ChainStore>(new ChainStore(pConfirmedChain, pCandidateChain, pSyncChain));
	return std::make_shared<Locked<ChainStore>>(Locked<ChainStore>(pChainStore));
}

void ChainStore::Commit()
{
	m_pSyncChain->Flush();
	m_pCandidateChain->Flush();
	m_pConfirmedChain->Flush();
}

BlockIndex* ChainStore::GetOrCreateIndex(const Hash& hash, const uint64_t height, BlockIndex* pPreviousIndex)
{
	BlockIndex* pSyncIndex = m_pSyncChain->GetByHeight(height);
	if (pSyncIndex != nullptr && pSyncIndex->GetHash() == hash)
	{
		return pSyncIndex;
	}

	BlockIndex* pCandidateIndex = m_pCandidateChain->GetByHeight(height);
	if (pCandidateIndex != nullptr && pCandidateIndex->GetHash() == hash)
	{
		return pCandidateIndex;
	}

	BlockIndex* pConfirmedIndex = m_pConfirmedChain->GetByHeight(height);
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == hash)
	{
		return pConfirmedIndex;
	}

	return new BlockIndex(hash, height, pPreviousIndex);
}

const BlockIndex* ChainStore::FindCommonIndex(const EChainType chainType1, const EChainType chainType2) const
{
	std::shared_ptr<const Chain> pChain1 = GetChain(chainType1);
	std::shared_ptr<const Chain> pChain2 = GetChain(chainType2);

	uint64_t height = (std::min)(pChain1->GetTip()->GetHeight(), pChain2->GetTip()->GetHeight());
	const BlockIndex* pChain1Index = pChain1->GetByHeight(height);
	const BlockIndex* pChain2Index = pChain2->GetByHeight(height);

	while (pChain1Index->GetHash() != pChain2Index->GetHash())
	{
		pChain1Index = pChain1Index->GetPrevious();
		pChain2Index = pChain2Index->GetPrevious();
	}

	return pChain1Index;
}

bool ChainStore::ReorgChain(const EChainType source, const EChainType destination, const uint64_t height)
{
	std::shared_ptr<Chain> pSourceChain = GetChain(source);
	std::shared_ptr<Chain> pDestinationChain = GetChain(destination);

	if (pSourceChain->GetTip()->GetHeight() < height)
	{
		return false;
	}

	const BlockIndex* pBlockIndex = FindCommonIndex(source, destination);
	if (pBlockIndex != nullptr)
	{
		const uint64_t commonHeight = pBlockIndex->GetHeight();
		if (pDestinationChain->Rewind(commonHeight))
		{
			for (uint64_t i = commonHeight + 1; i <= height; i++)
			{
				pDestinationChain->AddBlock(pSourceChain->GetByHeight(i));
			}

			return true;
		}
	}

	return false;
}

bool ChainStore::AddBlock(const EChainType source, const EChainType destination, const uint64_t height)
{
	std::shared_ptr<Chain> pSourceChain = GetChain(source);
	std::shared_ptr<Chain> pDestinationChain = GetChain(destination);

	if (pDestinationChain->GetTip()->GetHeight() + 1 == height)
	{
		if (pSourceChain->GetTip()->GetHeight() >= height)
		{
			return pDestinationChain->AddBlock(pSourceChain->GetByHeight(height));
		}
	}

	return false;
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
	else if (chainType == EChainType::SYNC)
	{
		return m_pSyncChain;
	}

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
	else if (chainType == EChainType::SYNC)
	{
		return m_pSyncChain;
	}

	throw std::exception();
}