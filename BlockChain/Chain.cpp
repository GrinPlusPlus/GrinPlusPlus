#include "Chain.h"

Chain::Chain(const EChainType chainType, BlockIndex* pGenesisBlock)
	: m_chainType(chainType), m_height(0)
{
	pGenesisBlock->AddChainType(m_chainType);
	m_indices.push_back(pGenesisBlock);
}

BlockIndex* Chain::GetByHeight(const uint64_t height)
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

bool Chain::AddBlock(BlockIndex* pBlockIndex)
{
	if (pBlockIndex->GetHeight() == (m_height + 1) && pBlockIndex->GetPrevious() == GetTip())
	{
		pBlockIndex->AddChainType(m_chainType);
		m_indices.push_back(pBlockIndex);
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
		m_height = lastHeight;
	}

	return true;
}