#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <Core/Models/BlockHeader.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class ShortId
{
public:
	//
	// Constructors
	//
	ShortId(CBigInteger<6>&& id);
	ShortId(const ShortId& other) = default;
	ShortId(ShortId&& other) noexcept = default;
	ShortId() = default;
	static ShortId Create(const CBigInteger<32>& hash, const CBigInteger<32>& blockHash, const uint64_t nonce);

	//
	// Destructor
	//
	~ShortId() = default;

	//
	// Operators
	//
	ShortId& operator=(const ShortId& other) = default;
	ShortId& operator=(ShortId&& other) noexcept = default;
	inline bool operator<(const ShortId& shortId) const { return m_id < shortId.m_id; }

	//
	// Getters
	//
	inline const CBigInteger<6>& GetId() const { return m_id; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static ShortId Deserialize(ByteBuffer& byteBuffer);

	//
	// Hashing
	//
	Hash GetHash() const;

private:
	CBigInteger<6> m_id;
};

static struct
{
	bool operator()(const ShortId& a, const ShortId& b) const
	{
		return a.GetHash() < b.GetHash();
	}
} SortShortIdsByHash;