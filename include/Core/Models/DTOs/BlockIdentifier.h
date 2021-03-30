#pragma once

#include <Crypto/Models/Hash.h>
#include <Core/Models/BlockHeader.h>
#include <json/json.h>

class BlockIdentifier
{
public:
	BlockIdentifier(const Hash& hash, const Hash& previousHash, const uint64_t height)
		: m_hash(hash), m_previousHash(previousHash), m_height(height) { }

	static BlockIdentifier FromHeader(const BlockHeader& header)
	{
		const Hash& headerHash = header.GetHash();
		const Hash& previousHash = header.GetPreviousHash();
		const uint64_t height = header.GetHeight();
		return BlockIdentifier(headerHash, previousHash, height);
	}

	const Hash& GetHash() const noexcept { return m_hash; }
	const Hash& GetPreviousHash() const noexcept { return m_previousHash; }
	uint64_t GetHeight() const noexcept { return m_height; }

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["hash"] = GetHash().ToHex();
		json["height"] = GetHeight();
		json["previous"] = GetPreviousHash().ToHex();
		return json;
	}

private:
	Hash m_hash;
	Hash m_previousHash;
	uint64_t m_height;
};