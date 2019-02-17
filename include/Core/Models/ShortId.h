#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

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

private:
	CBigInteger<6> m_id;
};