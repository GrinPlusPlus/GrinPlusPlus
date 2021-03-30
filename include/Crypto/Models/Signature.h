#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/Models/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class Signature : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	Signature() = default;
	Signature(CBigInteger<64>&& signatureBytes)
		: m_signatureBytes(std::move(signatureBytes)) { }
	Signature(const CBigInteger<64>& signatureBytes)
		: m_signatureBytes(signatureBytes) { }
	Signature(const Signature& other) = default;
	Signature(Signature&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~Signature() = default;

	//
	// Operators
	//
	Signature& operator=(const Signature& other) = default;
	Signature& operator=(Signature&& other) noexcept = default;
	bool operator==(const Signature& rhs) const noexcept { return m_signatureBytes == rhs.m_signatureBytes; }

	//
	// Getters
	//
	const CBigInteger<64>& GetSignatureBytes() const { return m_signatureBytes; }
	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return m_signatureBytes.GetData().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return m_signatureBytes.GetData().cend(); }
	const unsigned char* data() const { return m_signatureBytes.data(); }
	unsigned char* data() { return m_signatureBytes.data(); }

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

	std::string ToHex() const { return m_signatureBytes.ToHex(); }
	std::string Format() const override { return "RawSig{" + ToHex() + "}"; }

private:
	// The 64 byte Signature.
	CBigInteger<64> m_signatureBytes;
};

class CompactSignature : public Signature
{
public:
	CompactSignature() = default;
	CompactSignature(CBigInteger<64> && signatureBytes)
		: Signature(std::move(signatureBytes)) { }
	CompactSignature(const CBigInteger<64>& signatureBytes)
		: Signature(signatureBytes) { }
	virtual ~CompactSignature() = default;

	bool operator==(const CompactSignature& rhs) const noexcept { return GetSignatureBytes() == rhs.GetSignatureBytes(); }

	static CompactSignature Deserialize(ByteBuffer& byteBuffer)
	{
		return CompactSignature(byteBuffer.ReadBigInteger<64>());
	}

	std::string Format() const final { return "CompactSig{" + ToHex() + "}"; }
};

// TODO: Create RawSignature