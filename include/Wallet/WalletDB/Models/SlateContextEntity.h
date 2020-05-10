#pragma once

#include <Crypto/Crypto.h>
#include <Crypto/ED25519.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <optional>

static const uint8_t SLATE_CONTEXT_FORMAT = 0;

//
// Data Format:
// 32 bytes - blinding factor - encrypted using "blind" key
// 32 bytes - secret nonce - encrypted using "nonce" key
//
// Added in version 1:
// 1 byte - If 1, contains sender proof info.
//
// Sender proof info:
// VarStr - sender address keychain path
// 32 bytes - receiver address// 
//
class SlateContextEntity
{
public:
	SlateContextEntity(SecretKey&& secretKey, SecretKey&& secretNonce)
		: m_secretKey(std::move(secretKey)), m_secretNonce(std::move(secretNonce))
	{

	}

	const SecretKey& GetSecretKey() const { return m_secretKey; }
	const SecretKey& GetSecretNonce() const { return m_secretNonce; }

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

	static SlateContextEntity Decrypt(const std::vector<unsigned char>& encrypted, const SecureVector& masterSeed, const uuids::uuid& slateId)
	{
		ByteBuffer byteBuffer(encrypted);
		const uint8_t formatVersion = byteBuffer.ReadU8();
		if (formatVersion > SLATE_CONTEXT_FORMAT)
		{
			throw DESERIALIZATION_EXCEPTION_F(
				"Expected format <= {}, but was {}",
				SLATE_CONTEXT_FORMAT,
				formatVersion
			);
		}

		CBigInteger<32> decryptedBlind = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "blind");
		CBigInteger<32> decryptedNonce = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		return SlateContextEntity(SecretKey(std::move(decryptedBlind)), SecretKey(std::move(decryptedNonce)));
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