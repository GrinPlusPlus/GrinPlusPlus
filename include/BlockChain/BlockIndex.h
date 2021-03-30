#pragma once

#include <Crypto/Models/Hash.h>

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

	const Hash& GetHash() const noexcept { return m_hash; }
	uint64_t GetHeight() const noexcept { return m_height; }

private:
	Hash m_hash;
	uint64_t m_height;
};