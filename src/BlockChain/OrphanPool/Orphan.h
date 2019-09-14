#pragma once

#include <Core/Models/FullBlock.h>

struct Orphan
{
public:
	Orphan(const FullBlock& block)
		: m_block(block)
	{

	}
	
	inline bool operator<(const Orphan& rhs) const
	{
		if (this == &rhs)
		{
			return false;
		}

		const uint64_t height = this->m_block.GetBlockHeader().GetHeight();
		const uint64_t rhsHeight = rhs.m_block.GetBlockHeader().GetHeight();
		if (height != rhsHeight)
		{
			return height < rhsHeight;
		}
		
		return m_block.GetBlockHeader().GetHash() < rhs.m_block.GetBlockHeader().GetHash();
	}

	inline const FullBlock& GetBlock() const { return m_block; }

private:
	FullBlock m_block;
};