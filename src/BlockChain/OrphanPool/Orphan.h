#pragma once

#include <Core/Models/FullBlock.h>

struct Orphan
{
public:
	explicit Orphan(const FullBlock& block)
		: m_pBlock(std::make_shared<FullBlock>(block))
	{

	}
	
	inline bool operator<(const Orphan& rhs) const
	{
		if (this == &rhs)
		{
			return false;
		}

		const uint64_t height = this->m_pBlock->GetHeight();
		const uint64_t rhsHeight = rhs.m_pBlock->GetHeight();
		if (height != rhsHeight)
		{
			return height < rhsHeight;
		}
		
		return m_pBlock->GetHash() < rhs.m_pBlock->GetHash();
	}

	std::shared_ptr<const FullBlock> GetBlock() const noexcept { return m_pBlock; }
	const Hash& GetHash() const noexcept { return m_pBlock->GetHash(); }
	uint64_t GetHeight() const noexcept { return m_pBlock->GetHeight(); }

private:
	std::shared_ptr<FullBlock> m_pBlock;
};