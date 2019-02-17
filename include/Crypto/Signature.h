#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class Signature
{
public:
	//
	// Constructors
	//
	Signature(CBigInteger<64>&& signatureBytes)
		: m_signatureBytes(std::move(signatureBytes))
	{

	}
	Signature(const Signature& other) = default;
	Signature(Signature&& other) noexcept = default;

	//
	// Destructor
	//
	~Signature() = default;

	//
	// Operators
	//
	Signature& operator=(const Signature& other) = default;
	Signature& operator=(Signature&& other) noexcept = default;

	//
	// Getters
	//
	inline const CBigInteger<64>& GetSignatureBytes() const { return m_signatureBytes; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<64>(m_signatureBytes);
	}

	static Signature Deserialize(ByteBuffer& byteBuffer)
	{
		return Signature(byteBuffer.ReadBigInteger<64>());
	}

private:
	// The 64 byte Signature.
	CBigInteger<64> m_signatureBytes;
};