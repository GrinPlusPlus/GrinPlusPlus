#pragma once

#include <Crypto/Hash.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/BlockHeader.h>

class BlockIndex
{
public:
	BlockIndex(Hash&& hash, const uint64_t height)
		: m_hash(std::move(hash)), m_height(height)
	{

	}

	BlockIndex(const Hash& hash, const uint64_t height)
		: m_hash(hash), m_height(height)
	{

	}

	inline const Hash& GetHash() const { return m_hash; }
	inline uint64_t GetHeight() const { return m_height; }

private:
	Hash m_hash;
	uint64_t m_height;
};