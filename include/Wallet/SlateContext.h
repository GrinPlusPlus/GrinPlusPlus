#pragma once

#include <Crypto/Crypto.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>

static const uint8_t SLATE_CONTEXT_FORMAT = 0;

class SlateContext
{
public:
	SlateContext(SecretKey&& secretKey, SecretKey&& secretNonce)
		: m_secretKey(std::move(secretKey)), m_secretNonce(std::move(secretNonce))
	{

	}

	inline const SecretKey& GetSecretKey() const { return m_secretKey; }
	inline const SecretKey& GetSecretNonce() const { return m_secretNonce; }

	std::vector<unsigned char> Encrypt(const SecureVector& masterSeed, const uuids::uuid& slateId) const
	{
		const CBigInteger<32> encryptedBlind = m_secretKey.GetBytes() ^ DeriveXORKey(masterSeed, slateId, "blind");
		const CBigInteger<32> encryptedNonce = m_secretNonce.GetBytes() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		Serializer serializer;
		serializer.Append<uint8_t>(SLATE_CONTEXT_FORMAT);
		serializer.AppendBigInteger(encryptedBlind);
		serializer.AppendBigInteger(encryptedNonce);

		return serializer.GetBytes();
	}

	static SlateContext Decrypt(const std::vector<unsigned char>& encrypted, const SecureVector& masterSeed, const uuids::uuid& slateId)
	{
		ByteBuffer byteBuffer(encrypted);
		const uint8_t formatVersion = byteBuffer.ReadU8();
		if (formatVersion != SLATE_CONTEXT_FORMAT)
		{
			throw DeserializationException();
		}

		CBigInteger<32> decryptedBlind = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "blind");
		CBigInteger<32> decryptedNonce = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		return SlateContext(SecretKey(std::move(decryptedBlind)), SecretKey(std::move(decryptedNonce)));
	}

	static Hash DeriveXORKey(const SecureVector& masterSeed, const uuids::uuid& slateId, const std::string& extra)
	{
		Serializer serializer;
		serializer.AppendByteVector((const std::vector<unsigned char>&)masterSeed);
		serializer.AppendVarStr(uuids::to_string(slateId));
		serializer.AppendVarStr(extra);

		return Crypto::Blake2b(serializer.GetBytes());
	}

private:
	SecretKey m_secretKey;
	SecretKey m_secretNonce;
};