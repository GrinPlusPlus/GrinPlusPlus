#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

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

	std::string ToHex() const
	{
		return HexUtil::ConvertToHex(m_signatureBytes.GetData());
	}

private:
	// The 64 byte Signature.
	CBigInteger<64> m_signatureBytes;
};