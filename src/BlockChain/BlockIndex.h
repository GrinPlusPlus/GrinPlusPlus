#pragma once

#include <Crypto/Hash.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/BlockHeader.h>

class BlockIndex
{
public:
	BlockIndex(const Hash& hash, const uint64_t height)
		: m_hash(hash), m_height(height), m_chainTypeMask(0)
	{

	}

	inline const Hash& GetHash() const { return m_hash; }
	inline uint64_t GetHeight() const { return m_height; }

private:
	Hash m_hash;
	uint64_t m_height;
	uint8_t m_chainTypeMask;
	// TODO: Also save output & kernel MMR sizes
};