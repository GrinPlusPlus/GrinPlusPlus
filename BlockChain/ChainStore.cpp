#include "ChainStore.h"

#include <filesystem>
#include <FileUtil.h>
#include <vector>
#include <map>

ChainStore::ChainStore(const Config& config, BlockIndex* pGenesisIndex)
	: m_config(config),
	m_confirmedChain(EChainType::CONFIRMED, pGenesisIndex),
	m_candidateChain(EChainType::CANDIDATE, pGenesisIndex),
	m_syncChain(EChainType::SYNC, pGenesisIndex),
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

	const std::string dbPath = m_config.GetChainDirectory();

	bool success = true;
	if (!ReadChain(m_syncChain, dbPath + "sync.chain"))
	{
		success = false;
	}

	if (!ReadChain(m_candidateChain, dbPath + "candidate.chain"))
	{
		success = false;
	}

	if (!ReadChain(m_confirmedChain, dbPath + "confirmed.chain"))
	{
		success = false;
	}

	return false;
}

bool ChainStore::ReadChain(Chain& chain, const std::string& path)
{
	std::vector<unsigned char> data;
	if (FileUtil::ReadFile(path, data)) // TODO: Buffered read
	{
		BlockIndex* pPrevious = chain.GetByHeight(0);
		uint64_t height = 1; // Start at 1 to ignore genesis hash
		while (height * 32 < data.size())
		{
			const Hash hash = Hash(&data[height * 32]);
			BlockIndex* pIndex = GetOrCreateIndex(hash, height, pPrevious);
			chain.AddBlock(pIndex);

			pPrevious = pIndex;
			++height;
		}

		return true;
	}

	return false;
}

bool ChainStore::Flush()
{
	// Open file for writing
	const std::string path = m_config.GetChainDirectory();

	bool success = true;
	if (!WriteChain(m_syncChain, path + "sync.chain"))
	{
		success = false;
	}

	if (!WriteChain(m_candidateChain, path + "candidate.chain"))
	{
		success = false;
	}

	if (!WriteChain(m_confirmedChain, path + "confirmed.chain"))
	{
		success = false;
	}

	return success;
}

// TODO: Use mem-mapped file
bool ChainStore::WriteChain(Chain& chain, const std::string& path)
{
	std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		return false;
	}

	bool success = false;

	// For each block, write hash to file
	const uint64_t height = chain.GetTip()->GetHeight();
	for (uint64_t i = 0; i <= height; i++)
	{
		BlockIndex* pIndex = chain.GetByHeight(i);
		if (pIndex == nullptr)
		{
			break;
		}

		file.write((const char*)&pIndex->GetHash().GetData()[0], 32); // TODO: Use a buffered write.
	}

	// Close file
	file.close();
	return success;
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

	uint64_t height = std::min(chain1.GetTip()->GetHeight(), chain2.GetTip()->GetHeight());
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