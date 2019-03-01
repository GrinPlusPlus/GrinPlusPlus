#pragma once

#include <Crypto/BigInteger.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>

static const uint8_t ENCRYPTED_SEED_FORMAT = 0;

class EncryptedSeed
{
public:
	EncryptedSeed(CBigInteger<16>&& iv, CBigInteger<8>&& salt, std::vector<unsigned char>&& encryptedSeedBytes)
		: m_iv(std::move(iv)), m_salt(std::move(salt)), m_encryptedSeedBytes(std::move(encryptedSeedBytes))
	{

	}

	inline const CBigInteger<16>& GetIV() const { return m_iv; }
	inline const CBigInteger<8>& GetSalt() const { return m_salt; }
	inline const std::vector<unsigned char>& GetEncryptedSeedBytes() const { return m_encryptedSeedBytes; }

	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint8_t>(ENCRYPTED_SEED_FORMAT);
		serializer.AppendBigInteger<16>(m_iv);
		serializer.AppendBigInteger<8>(m_salt);
		serializer.AppendByteVector(m_encryptedSeedBytes);
	}

	static EncryptedSeed Deserialize(ByteBuffer& byteBuffer)
	{
		if (byteBuffer.ReadU8() != ENCRYPTED_SEED_FORMAT)
		{
			throw DeserializationException();
		}

		CBigInteger<16> iv = byteBuffer.ReadBigInteger<16>();
		CBigInteger<8> salt = byteBuffer.ReadBigInteger<8>();
		std::vector<unsigned char> encryptedSeedBytes = byteBuffer.ReadVector(byteBuffer.GetRemainingSize());

		return EncryptedSeed(std::move(iv), std::move(salt), std::move(encryptedSeedBytes));
	}

private:
	CBigInteger<16> m_iv;
	CBigInteger<8> m_salt;
	std::vector<unsigned char> m_encryptedSeedBytes;
};