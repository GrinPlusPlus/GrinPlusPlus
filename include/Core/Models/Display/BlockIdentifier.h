#pragma once

#include <Crypto/Hash.h>
#include <Core/Models/BlockHeader.h>

class BlockIdentifier
{
public:
	BlockIdentifier(const Hash& hash, const Hash& previousHash, const uint64_t height)
		: m_hash(hash), m_previousHash(previousHash), m_height(height)
	{

	}

	static BlockIdentifier FromHeader(const BlockHeader& header)
	{
		const Hash& headerHash = header.GetHash();
		const Hash& previousHash = header.GetPreviousBlockHash();
		const uint64_t height = header.GetHeight();
		return BlockIdentifier(headerHash, previousHash, height);
	}

	const Hash& GetHash() const { return m_hash; }
	const Hash& GetPreviousHash() const { return m_previousHash; }
	uint64_t GetHeight() const { return m_height; }

private:
	Hash m_hash;
	Hash m_previousHash;
	uint64_t m_height;
};