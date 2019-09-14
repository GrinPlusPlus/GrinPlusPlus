#include "Chain.h"
#include "ChainStore.h"

Chain::Chain(const EChainType chainType, const std::string& path, BlockIndex* pGenesisBlock)
	: m_chainType(chainType), m_dataFile(path), m_height(0)
{
	pGenesisBlock->AddChainType(m_chainType);
	m_indices.push_back(pGenesisBlock);
}

bool Chain::Load(ChainStore& chainStore)
{
	bool loaded = m_dataFile.Load();
	if (m_dataFile.GetSize() == 0)
	{
		m_dataFile.AddData(m_indices[0]->GetHash().GetData());
	}
	else
	{
		BlockIndex* pPrevious = GetByHeight(0);
		while ((m_height + 1) < m_dataFile.GetSize())
		{
			std::vector<unsigned char> hashVec;
			m_dataFile.GetDataAt(m_height + 1, hashVec);

			const Hash hash = Hash(std::move(hashVec));
			BlockIndex* pBlockIndex = chainStore.GetOrCreateIndex(hash, m_height + 1, pPrevious);

			pBlockIndex->AddChainType(m_chainType);
			m_indices.push_back(pBlockIndex);

			pPrevious = pBlockIndex;
			++m_height;
		}
	}

	return loaded;
}

BlockIndex* Chain::GetByHeight(const uint64_t height)
{
	if (m_height >= height)
	{
		return m_indices[height];
	}

	return nullptr;
}

const BlockIndex* Chain::GetByHeight(const uint64_t height) const
{
	if (m_height >= height)
	{
		return m_indices[height];
	}

	return nullptr;
}

BlockIndex* Chain::GetTip()
{
	return m_indices[m_height];
}

const BlockIndex* Chain::GetTip() const
{
	return m_indices[m_height];
}

bool Chain::AddBlock(BlockIndex* pBlockIndex)
{
	if (pBlockIndex->GetHeight() == (m_height + 1) && pBlockIndex->GetPrevious() == GetTip())
	{
		pBlockIndex->AddChainType(m_chainType);
		m_indices.push_back(pBlockIndex);
		m_dataFile.AddData(pBlockIndex->GetHash().GetData());
		m_height++;
		return true;
	}

	return false;
}

bool Chain::Rewind(const uint64_t lastHeight)
{
	if (m_height < lastHeight)
	{
		return false;
	}

	if (m_height > lastHeight)
	{
		for (uint64_t i = lastHeight + 1; i <= m_height; i++)
		{
			BlockIndex* pBlockIndex = m_indices[i];
			pBlockIndex->RemoveChainType(m_chainType);
			if (pBlockIndex->IsSafeToDelete())
			{
				delete pBlockIndex;
			}
		}

		m_indices.erase(m_indices.begin() + lastHeight + 1, m_indices.end());
		m_dataFile.Rewind(lastHeight + 1);
		m_height = lastHeight;
	}

	return true;
}

bool Chain::Flush()
{
	return m_dataFile.Flush();
}