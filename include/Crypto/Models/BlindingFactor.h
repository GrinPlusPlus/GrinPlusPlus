#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/Models/Hash.h>
#include <Crypto/Models/SecretKey.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class BlindingFactor : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	BlindingFactor() = default;

	BlindingFactor(Hash&& blindingFactorBytes)
		: m_blindingFactorBytes(std::move(blindingFactorBytes))
	{

	}
	BlindingFactor(const Hash& blindingFactorBytes)
		: m_blindingFactorBytes(blindingFactorBytes)
	{

	}
	BlindingFactor(const BlindingFactor& other) = default;
	BlindingFactor(BlindingFactor&& other) noexcept = default;

	//
	// Destructor
	//
	~BlindingFactor() = default;

	//
	// Operators
	//
	BlindingFactor& operator=(const BlindingFactor& other) = default;
	BlindingFactor& operator=(BlindingFactor&& other) noexcept = default;
	bool operator<(const BlindingFactor& rhs) const { return m_blindingFactorBytes < rhs.GetBytes(); }
	bool operator!=(const BlindingFactor& rhs) const { return m_blindingFactorBytes != rhs.GetBytes(); }
	bool operator==(const BlindingFactor& rhs) const { return m_blindingFactorBytes == rhs.GetBytes(); }

	//
	// Getters
	//
	const CBigInteger<32>& GetBytes() const { return m_blindingFactorBytes; }
	const std::vector<unsigned char>& GetVec() const { return m_blindingFactorBytes.GetData(); }
	const unsigned char* data() const { return m_blindingFactorBytes.data(); }
	std::string ToHex() const { return m_blindingFactorBytes.ToHex(); }
	bool IsNull() const noexcept { return m_blindingFactorBytes == CBigInteger<32>{}; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<32>(m_blindingFactorBytes);
	}

	static BlindingFactor Deserialize(ByteBuffer& byteBuffer)
	{
		return BlindingFactor(byteBuffer.ReadBigInteger<32>());
	}

	static BlindingFactor FromHex(const std::string& hex)
	{
		return BlindingFactor(CBigInteger<32>::FromHex(hex));
	}

	std::string Format() const final { return "BlindingFactor{" + ToHex() + "}"; }

	//
	// Converts BlindingFactor to SecretKey.
	// WARNING: BlindingFactor is unusable after calling this.
	//
	SecretKey ToSecretKey()
	{
		return SecretKey(std::move(m_blindingFactorBytes));
	}

private:
	// The 32 byte blinding factor.
	Hash m_blindingFactorBytes;
};