#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/BigInteger.h>
#include <Common/Util/HexUtil.h>
#include <Core/Traits/Printable.h>
#include <Core/Traits/Serializable.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

static const int MAX_PROOF_SIZE = 675;

class RangeProof : public Traits::IPrintable, public Traits::ISerializable
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
	virtual ~RangeProof() = default;

	//
	// Operators
	//
	RangeProof& operator=(const RangeProof& other) = default;
	RangeProof& operator=(RangeProof&& other) noexcept = default;

	//
	// Getters
	//
	const std::vector<unsigned char>& GetProofBytes() const noexcept { return m_proofBytes; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final
	{
		serializer.Append<uint64_t>(m_proofBytes.size());
		serializer.AppendByteVector(m_proofBytes);
	}

	static RangeProof Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t proofSize = byteBuffer.ReadU64();
		if (proofSize > MAX_PROOF_SIZE)
		{
			throw DESERIALIZATION_EXCEPTION_F("Proof of size {} exceeds the maximum", proofSize);
		}

		return RangeProof(byteBuffer.ReadVector(proofSize));
	}

	//
	// Traits
	//
	virtual std::string Format() const override final { return HexUtil::ConvertToHex(m_proofBytes); }

private:
	// The proof itself, at most 675 bytes long.
	std::vector<unsigned char> m_proofBytes;
};
