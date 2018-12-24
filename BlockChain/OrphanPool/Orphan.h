#pragma once

#include <Core/FullBlock.h>

struct Orphan
{
public:
	Orphan(const FullBlock& block)
		: m_block(block)
	{

	}

	inline const FullBlock& GetBlock() const { return m_block; }

private:
	FullBlock m_block;
};