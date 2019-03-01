#pragma once

#include <Crypto/Crypto.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>

static const uint8_t SLATE_CONTEXT_FORMAT = 0;

class SlateContext
{
public:
	SlateContext(BlindingFactor&& secretKey, BlindingFactor&& secretNonce)
		: m_secretKey(std::move(secretKey)), m_secretNonce(std::move(secretNonce))
	{

	}

	inline const BlindingFactor& GetSecretKey() const { return m_secretKey; }
	inline const BlindingFactor& GetSecretNonce() const { return m_secretNonce; }

	std::vector<unsigned char> Encrypt(const CBigInteger<32>& masterSeed, const uuids::uuid& slateId) const
	{
		const CBigInteger<32> encryptedBlind = m_secretKey.GetBytes() ^ DeriveXORKey(masterSeed, slateId, "blind");
		const CBigInteger<32> encryptedNonce = m_secretNonce.GetBytes() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		Serializer serializer;
		serializer.Append<uint8_t>(SLATE_CONTEXT_FORMAT);
		serializer.AppendBigInteger(encryptedBlind);
		serializer.AppendBigInteger(encryptedNonce);

		return serializer.GetBytes();
	}

	static SlateContext Decrypt(const std::vector<unsigned char>& encrypted, const CBigInteger<32>& masterSeed, const uuids::uuid& slateId)
	{
		ByteBuffer byteBuffer(encrypted);
		const uint8_t formatVersion = byteBuffer.ReadU8();
		if (formatVersion != SLATE_CONTEXT_FORMAT)
		{
			throw DeserializationException();
		}

		CBigInteger<32> decryptedBlind = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "blind");
		CBigInteger<32> decryptedNonce = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		return SlateContext(BlindingFactor(std::move(decryptedBlind)), BlindingFactor(std::move(decryptedNonce)));
	}

private:
	static Hash DeriveXORKey(const CBigInteger<32>& masterSeed, const uuids::uuid& slateId, const std::string& extra)
	{
		Serializer serializer;
		serializer.AppendBigInteger(masterSeed);
		serializer.AppendVarStr(uuids::to_string(slateId));
		serializer.AppendVarStr(extra);

		return Crypto::Blake2b(serializer.GetBytes());
	}

	BlindingFactor m_secretKey;
	BlindingFactor m_secretNonce;
};