#pragma once

#include <Crypto/ED25519.h>
#include <Crypto/Signature.h>
#include <Core/Util/JsonUtil.h>
#include <optional>

class SlatePaymentProof
{
public:
	static SlatePaymentProof Create(const ed25519_public_key_t& senderAddress, const ed25519_public_key_t& receiverAddress)
	{
		return SlatePaymentProof(senderAddress, receiverAddress, std::nullopt);
	}

	bool operator==(const SlatePaymentProof& rhs) const noexcept
	{
		return m_senderAddress == rhs.m_senderAddress &&
			m_receiverAddress == rhs.m_receiverAddress &&
			m_receiverSignatureOpt == rhs.m_receiverSignatureOpt;
	}

	const ed25519_public_key_t& GetSenderAddress() const { return m_senderAddress; }
	const ed25519_public_key_t& GetReceiverAddress() const { return m_receiverAddress; }
	const std::optional<Signature>& GetReceiverSignature() const { return m_receiverSignatureOpt; }
	void AddSignature(Signature&& signature) { m_receiverSignatureOpt = std::make_optional(std::move(signature)); }

	Json::Value ToJSON() const
	{
		Json::Value proofJSON;
		proofJSON["saddr"] = m_senderAddress.Format();
		proofJSON["raddr"] = m_receiverAddress.Format();
		if (m_receiverSignatureOpt.has_value())
		{
			proofJSON["rsig"] = m_receiverSignatureOpt.value().ToHex();
		}

		return proofJSON;
	}

	static SlatePaymentProof FromJSON(const Json::Value& paymentProofJSON)
	{
		ed25519_public_key_t senderAddress;
		senderAddress.pubkey = JsonUtil::ConvertToVector(JsonUtil::GetRequiredField(paymentProofJSON, "saddr"), ED25519_PUBKEY_LEN);

		ed25519_public_key_t receiverAddress;
		receiverAddress.pubkey = JsonUtil::ConvertToVector(JsonUtil::GetRequiredField(paymentProofJSON, "raddr"), ED25519_PUBKEY_LEN);

		std::optional<Signature> receiverSignatureOpt = JsonUtil::GetSignatureOpt(paymentProofJSON, "rsig");

		return SlatePaymentProof(senderAddress, receiverAddress, receiverSignatureOpt);
	}

	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<32>(CBigInteger<32>(m_senderAddress.pubkey));
		serializer.AppendBigInteger<32>(CBigInteger<32>(m_receiverAddress.pubkey));

		if (m_receiverSignatureOpt.has_value())
		{
			serializer.Append<uint8_t>(1);
			serializer.AppendBigInteger<64>(m_receiverSignatureOpt.value().GetSignatureBytes());
		}
		else
		{
			serializer.Append<uint8_t>(0);
		}
	}

	static SlatePaymentProof Deserialize(ByteBuffer& byteBuffer)
	{
		ed25519_public_key_t senderAddress;
		senderAddress.pubkey = byteBuffer.ReadBigInteger<32>().GetData();

		ed25519_public_key_t receiverAddress;
		receiverAddress.pubkey = byteBuffer.ReadBigInteger<32>().GetData();

		std::optional<Signature> receiverSignatureOpt = std::nullopt;

		const uint8_t hasSignature = byteBuffer.ReadU8();
		if (hasSignature == 1)
		{
			receiverSignatureOpt = std::make_optional(Signature::Deserialize(byteBuffer));
		}

		return SlatePaymentProof(senderAddress, receiverAddress, receiverSignatureOpt);
	}

private:
	SlatePaymentProof(
		const ed25519_public_key_t& senderAddress,
		const ed25519_public_key_t& receiverAddress,
		const std::optional<Signature>& receiverSignatureOpt)
		: m_senderAddress(senderAddress), m_receiverAddress(receiverAddress), m_receiverSignatureOpt(receiverSignatureOpt)
	{

	}

	ed25519_public_key_t m_senderAddress;
	ed25519_public_key_t m_receiverAddress;
	std::optional<Signature> m_receiverSignatureOpt;
};