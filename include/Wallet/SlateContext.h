#pragma once

#include <Crypto/Crypto.h>
#include <Crypto/ED25519.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <optional>

static const uint8_t SLATE_CONTEXT_FORMAT = 1;

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
class SlateContext
{
public:
	SlateContext(
		SecretKey&& secretKey,
		SecretKey&& secretNonce,
		std::optional<KeyChainPath>&& senderAddressOpt,
		std::optional<ed25519_public_key_t>&& receiverAddressOpt)
		: m_secretKey(std::move(secretKey)),
		m_secretNonce(std::move(secretNonce)),
		m_senderAddressOpt(std::move(senderAddressOpt)),
		m_receiverAddressOpt(std::move(receiverAddressOpt))
	{

	}

	const SecretKey& GetSecretKey() const { return m_secretKey; }
	const SecretKey& GetSecretNonce() const { return m_secretNonce; }
	const std::optional<KeyChainPath>& GetSenderAddress() const { return m_senderAddressOpt; }
	const std::optional<ed25519_public_key_t>& GetReceiverAddress() const { return m_receiverAddressOpt; }

	std::vector<unsigned char> Encrypt(const SecureVector& masterSeed, const uuids::uuid& slateId) const
	{
		const CBigInteger<32> encryptedBlind = m_secretKey.GetBytes() ^ DeriveXORKey(masterSeed, slateId, "blind");
		const CBigInteger<32> encryptedNonce = m_secretNonce.GetBytes() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		Serializer serializer;
		serializer.Append<uint8_t>(SLATE_CONTEXT_FORMAT);
		serializer.AppendBigInteger(encryptedBlind);
		serializer.AppendBigInteger(encryptedNonce);

		serializer.Append<uint8_t>(m_senderAddressOpt.has_value() ? 1 : 0);
		if (m_senderAddressOpt.has_value())
		{
			serializer.AppendVarStr(m_senderAddressOpt.value().Format());
			serializer.AppendBigInteger(CBigInteger<32>(m_receiverAddressOpt.value().pubkey));
		}

		return serializer.GetBytes();
	}

	static SlateContext Decrypt(const std::vector<unsigned char>& encrypted, const SecureVector& masterSeed, const uuids::uuid& slateId)
	{
		ByteBuffer byteBuffer(encrypted);
		const uint8_t formatVersion = byteBuffer.ReadU8();
		if (formatVersion < SLATE_CONTEXT_FORMAT)
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		CBigInteger<32> decryptedBlind = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "blind");
		CBigInteger<32> decryptedNonce = byteBuffer.ReadBigInteger<32>() ^ DeriveXORKey(masterSeed, slateId, "nonce");

		std::optional<KeyChainPath> senderAddress = std::nullopt;
		std::optional<ed25519_public_key_t> receiverAddressOpt = std::nullopt;
		if (formatVersion >= 1)
		{
			uint8_t type = byteBuffer.ReadU8();
			if (type == 1)
			{
				senderAddress = std::make_optional<KeyChainPath>(KeyChainPath::FromString(byteBuffer.ReadVarStr()));

				ed25519_public_key_t receiverAddress;
				receiverAddress.pubkey = byteBuffer.ReadBigInteger<32>().GetData();
				receiverAddressOpt = std::make_optional<ed25519_public_key_t>(receiverAddress);
			}
		}

		return SlateContext(
			SecretKey(std::move(decryptedBlind)),
			SecretKey(std::move(decryptedNonce)),
			std::move(senderAddress),
			std::move(receiverAddressOpt)
		);
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
	std::optional<KeyChainPath> m_senderAddressOpt;
	std::optional<ed25519_public_key_t> m_receiverAddressOpt;
};