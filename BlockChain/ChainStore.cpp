#include "ChainStore.h"
#include <Common/Util/FileUtil.h>
#include <Infrastructure/Logger.h>
#include <vector>
#include <map>

ChainStore::ChainStore(const Config& config, BlockIndex* pGenesisIndex)
	: m_config(config),
	m_confirmedChain(EChainType::CONFIRMED, config.GetChainDirectory() + "confirmed.chain", pGenesisIndex),
	m_candidateChain(EChainType::CANDIDATE, config.GetChainDirectory() + "candidate.chain", pGenesisIndex),
	m_syncChain(EChainType::SYNC, config.GetChainDirectory() + "candidate.chain", pGenesisIndex),
	m_loaded(false)
{

}

bool ChainStore::Load()
{
	if (m_loaded)
	{
		return true;
	}

	m_loaded = true;

	LoggerAPI::LogTrace("ChainStore::Load - Loading Chain");
	bool success = true;
	if (!m_syncChain.Load(*this))
	{
		LoggerAPI::LogInfo("ChainStore::Load - Failed to load sync chain");
		success = false;
	}
	
	if (!m_candidateChain.Load(*this))
	{
		LoggerAPI::LogInfo("ChainStore::Load - Failed to load candidate chain");
		success = false;
	}

	if (!m_confirmedChain.Load(*this))
	{
		LoggerAPI::LogInfo("ChainStore::Load - Failed to load confirmed chain");
		success = false;
	}

	return success;
}

bool ChainStore::Flush()
{
	return /*m_syncChain.Flush() &&*/ m_candidateChain.Flush() && m_confirmedChain.Flush();
}

BlockIndex* ChainStore::GetOrCreateIndex(const Hash& hash, const uint64_t height, BlockIndex* pPreviousIndex)
{
	BlockIndex* pSyncIndex = m_syncChain.GetByHeight(height);
	if (pSyncIndex != nullptr && pSyncIndex->GetHash() == hash)
	{
		return pSyncIndex;
	}

	BlockIndex* pCandidateIndex = m_candidateChain.GetByHeight(height);
	if (pCandidateIndex != nullptr && pCandidateIndex->GetHash() == hash)
	{
		return pCandidateIndex;
	}

	BlockIndex* pConfirmedIndex = m_confirmedChain.GetByHeight(height);
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == hash)
	{
		return pConfirmedIndex;
	}

	return new BlockIndex(hash, height, pPreviousIndex);
}

BlockIndex* ChainStore::FindCommonIndex(const EChainType chainType1, const EChainType chainType2)
{
	Chain& chain1 = GetChain(chainType1);
	Chain& chain2 = GetChain(chainType2);

	uint64_t height = (std::min)(chain1.GetTip()->GetHeight(), chain2.GetTip()->GetHeight());
	BlockIndex* pChain1Index = chain1.GetByHeight(height);
	BlockIndex* pChain2Index = chain2.GetByHeight(height);

	while (pChain1Index->GetHash() != pChain2Index->GetHash())
	{
		pChain1Index = pChain1Index->GetPrevious();
		pChain2Index = pChain2Index->GetPrevious();
	}

	return pChain1Index;
}

bool ChainStore::ReorgChain(const EChainType source, const EChainType destination, const uint64_t height)
{
	Chain& sourceChain = GetChain(source);
	Chain& destinationChain = GetChain(destination);

	if (sourceChain.GetTip()->GetHeight() < height)
	{
		return false;
	}

	BlockIndex* pBlockIndex = FindCommonIndex(source, destination);
	if (pBlockIndex != nullptr)
	{
		const uint64_t commonHeight = pBlockIndex->GetHeight();
		if (destinationChain.Rewind(commonHeight))
		{
			for (uint64_t i = commonHeight + 1; i <= height; i++)
			{
				destinationChain.AddBlock(sourceChain.GetByHeight(i));
			}

			return true;
		}
	}

	return false;
}

bool ChainStore::AddBlock(const EChainType source, const EChainType destination, const uint64_t height)
{
	Chain& sourceChain = GetChain(source);
	Chain& destinationChain = GetChain(destination);

	if (destinationChain.GetTip()->GetHeight() + 1 == height)
	{
		if (sourceChain.GetTip()->GetHeight() >= height)
		{
			return destinationChain.AddBlock(sourceChain.GetByHeight(height));
		}
	}

	return false;
}

Chain& ChainStore::GetChain(const EChainType chainType)
{
	if (chainType == EChainType::CONFIRMED)
	{
		return m_confirmedChain;
	}
	else if (chainType == EChainType::CANDIDATE)
	{
		return m_candidateChain;
	}
	else if (chainType == EChainType::SYNC)
	{
		return m_syncChain;
	}

	throw std::exception();
}

const Chain& ChainStore::GetChain(const EChainType chainType) const
{
	if (chainType == EChainType::CONFIRMED)
	{
		return m_confirmedChain;
	}
	else if (chainType == EChainType::CANDIDATE)
	{
		return m_candidateChain;
	}
	else if (chainType == EChainType::SYNC)
	{
		return m_syncChain;
	}

	throw std::exception();
}