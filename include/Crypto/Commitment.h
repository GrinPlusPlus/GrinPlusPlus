#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <BigInteger.h>
#include <BitUtil.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

class Commitment
{
public:
	//
	// Constructors
	//
	Commitment(CBigInteger<33>&& commitmentBytes)
		: m_commitmentBytes(std::move(commitmentBytes))
	{

	}
	Commitment(const Commitment& other) = default;
	Commitment(Commitment&& other) noexcept = default;

	//
	// Destructor
	//
	~Commitment() = default;

	//
	// Operators
	//
	Commitment& operator=(const Commitment& other) = default;
	Commitment& operator=(Commitment&& other) noexcept = default;
	inline bool operator<(const Commitment& rhs) const { return m_commitmentBytes < rhs.GetCommitmentBytes(); }
	inline bool operator!=(const Commitment& rhs) const { return m_commitmentBytes != rhs.GetCommitmentBytes(); }
	inline bool operator==(const Commitment& rhs) const { return m_commitmentBytes == rhs.GetCommitmentBytes(); }

	//
	// Getters
	//
	inline const CBigInteger<33>& GetCommitmentBytes() const { return m_commitmentBytes; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<33>(m_commitmentBytes);
	}

	static Commitment Deserialize(ByteBuffer& byteBuffer)
	{
		return Commitment(byteBuffer.ReadBigInteger<33>());
	}

private:
	// The 33 byte commitment.
	CBigInteger<33> m_commitmentBytes;
};

namespace std
{
	template<>
	struct hash<Commitment>
	{
		size_t operator()(const Commitment& commitment) const
		{
			const std::vector<unsigned char>& bytes = commitment.GetCommitmentBytes().GetData();
			return BitUtil::ConvertToU64(bytes[0], bytes[4], bytes[8], bytes[12], bytes[16], bytes[20], bytes[24], bytes[28]);
		}
	};
}