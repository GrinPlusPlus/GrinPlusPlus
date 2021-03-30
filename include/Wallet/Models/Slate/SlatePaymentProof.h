#pragma once

#include <Crypto/ED25519.h>
#include <Crypto/Models/Signature.h>
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
	const std::optional<ed25519_signature_t>& GetReceiverSignature() const { return m_receiverSignatureOpt; }
	void AddSignature(ed25519_signature_t&& signature) { m_receiverSignatureOpt = std::make_optional(std::move(signature)); }

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
		senderAddress.bytes = JsonUtil::ConvertToVector(JsonUtil::GetRequiredField(paymentProofJSON, "saddr"), ED25519_PUBKEY_LEN);

		ed25519_public_key_t receiverAddress;
		receiverAddress.bytes = JsonUtil::ConvertToVector(JsonUtil::GetRequiredField(paymentProofJSON, "raddr"), ED25519_PUBKEY_LEN);

		std::optional<CBigInteger<64>> receiverSignatureOpt = JsonUtil::GetBigIntegerOpt<64>(paymentProofJSON, "rsig");

		return SlatePaymentProof(
			senderAddress,
			receiverAddress,
			receiverSignatureOpt.has_value() ? std::make_optional(ed25519_signature_t{ receiverSignatureOpt.value() }) : std::nullopt
		);
	}

	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<32>(m_senderAddress.bytes);
		serializer.AppendBigInteger<32>(m_receiverAddress.bytes);

		if (m_receiverSignatureOpt.has_value())
		{
			serializer.Append<uint8_t>(1);
			serializer.AppendBigInteger<64>(m_receiverSignatureOpt.value().bytes);
		}
		else
		{
			serializer.Append<uint8_t>(0);
		}
	}

	static SlatePaymentProof Deserialize(ByteBuffer& byteBuffer)
	{
		ed25519_public_key_t senderAddress = byteBuffer.ReadBigInteger<32>();
		ed25519_public_key_t receiverAddress = byteBuffer.ReadBigInteger<32>();

		std::optional<ed25519_signature_t> receiverSignatureOpt = std::nullopt;

		const uint8_t hasSignature = byteBuffer.ReadU8();
		if (hasSignature == 1)
		{
			receiverSignatureOpt = std::make_optional(ed25519_signature_t{ byteBuffer.ReadBigInteger<64>() });
		}

		return SlatePaymentProof(senderAddress, receiverAddress, receiverSignatureOpt);
	}

private:
	SlatePaymentProof(
		const ed25519_public_key_t& senderAddress,
		const ed25519_public_key_t& receiverAddress,
		const std::optional<ed25519_signature_t>& receiverSignatureOpt)
		: m_senderAddress(senderAddress),
		m_receiverAddress(receiverAddress),
		m_receiverSignatureOpt(receiverSignatureOpt)
	{

	}

	ed25519_public_key_t m_senderAddress;
	ed25519_public_key_t m_receiverAddress;
	std::optional<ed25519_signature_t> m_receiverSignatureOpt;
};