#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

static const int MAX_PROOF_SIZE = 675; // TODO: Find central location for this.

class RangeProof
{
public:
	//
	// Constructors
	//
	RangeProof(std::vector<unsigned char>&& proofBytes)
		: m_proofBytes(std::move(proofBytes))
	{

	}
	RangeProof(const RangeProof& other) = default;
	RangeProof(RangeProof&& other) noexcept = default;

	//
	// Destructor
	//
	~RangeProof() = default;

	//
	// Operators
	//
	RangeProof& operator=(const RangeProof& other) = default;
	RangeProof& operator=(RangeProof&& other) noexcept = default;

	//
	// Getters
	//
	inline const std::vector<unsigned char>& GetProofBytes() const { return m_proofBytes; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint64_t>(m_proofBytes.size());
		serializer.AppendByteVector(m_proofBytes);
	}

	static RangeProof Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t proofSize = byteBuffer.ReadU64();
		if (proofSize > MAX_PROOF_SIZE)
		{
			throw DeserializationException();
		}

		return RangeProof(std::move(byteBuffer.ReadVector(proofSize)));
	}

private:
	// The proof itself, at most 675 bytes long.
	std::vector<unsigned char> m_proofBytes;
};