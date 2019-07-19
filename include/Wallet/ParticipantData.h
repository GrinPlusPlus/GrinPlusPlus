#pragma once

#include <Crypto/Commitment.h>
#include <Crypto/PublicKey.h>
#include <Crypto/Signature.h>
#include <Core/Util/JsonUtil.h>
#include <json/json.h>
#include <stdint.h>
#include <optional>

class ParticipantData
{
public:
	ParticipantData(const uint64_t participantId, const PublicKey& publicBlindExcess, const PublicKey& publicNonce)
		: m_participantId(participantId), m_publicBlindExcess(publicBlindExcess), m_publicNonce(publicNonce)
	{

	}

	ParticipantData(const uint64_t participantId, PublicKey&& publicBlindExcess, PublicKey&& publicNonce, std::optional<Signature>&& partialSigOpt, std::optional<std::string>&& messageOpt, std::optional<Signature>&& messageSigOpt)
		: m_participantId(participantId), m_publicBlindExcess(publicBlindExcess), m_publicNonce(publicNonce), m_partialSignatureOpt(std::move(partialSigOpt)), m_messageOpt(std::move(messageOpt)), m_messageSignatureOpt(std::move(messageSigOpt))
	{

	}

	inline uint64_t GetParticipantId() const { return m_participantId; }
	inline const PublicKey& GetPublicBlindExcess() const { return m_publicBlindExcess; }
	inline const PublicKey& GetPublicNonce() const { return m_publicNonce; }
	inline const std::optional<Signature>& GetPartialSignature() const { return m_partialSignatureOpt; }
	inline const std::optional<std::string>& GetMessageText() const { return m_messageOpt; }
	inline const std::optional<Signature>& GetMessageSignature() const { return m_messageSignatureOpt; }

	inline void AddPartialSignature(const Signature& signature) { m_partialSignatureOpt = std::make_optional<Signature>(signature); }
	inline void AddMessage(const std::string& message, const Signature& messageSignature)
	{
		m_messageOpt = std::make_optional<std::string>(message);
		m_messageSignatureOpt = std::make_optional<Signature>(messageSignature);
	}

	inline Json::Value ToJSON(const bool hex) const
	{
		Json::Value participantJSON;
		participantJSON["id"] = std::to_string(GetParticipantId());
		participantJSON["public_blind_excess"] = JsonUtil::ConvertToJSON(GetPublicBlindExcess().GetCompressedBytes().GetData(), hex);
		participantJSON["public_nonce"] = JsonUtil::ConvertToJSON(GetPublicNonce().GetCompressedBytes().GetData(), hex);
		participantJSON["part_sig"] = JsonUtil::ConvertToJSON(GetPartialSignature(), hex);
		participantJSON["message"] = JsonUtil::ConvertToJSON(GetMessageText());
		participantJSON["message_sig"] = JsonUtil::ConvertToJSON(GetMessageSignature(), hex);
		return participantJSON;
	}

	static ParticipantData FromJSON(const Json::Value& participantJSON, const bool hex)
	{
		const uint64_t participantId = JsonUtil::GetRequiredUInt64(participantJSON, "id");
		PublicKey publicBlindExcess = JsonUtil::GetPublicKey(participantJSON, "public_blind_excess", hex);
		PublicKey publicNonce = JsonUtil::GetPublicKey(participantJSON, "public_nonce", hex);
		std::optional<Signature> partialSigOpt = JsonUtil::GetSignatureOpt(participantJSON, "part_sig", hex);
		std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(participantJSON, "message");
		std::optional<Signature> messageSigOpt = JsonUtil::GetSignatureOpt(participantJSON, "message_sig", hex);

		return ParticipantData(participantId, std::move(publicBlindExcess), std::move(publicNonce), std::move(partialSigOpt), std::move(messageOpt), std::move(messageSigOpt));
	}

private:
	// Id of participant in the transaction. (For now, 0=sender, 1=rec)
	uint64_t m_participantId;
	// Public key corresponding to private blinding factor
	PublicKey m_publicBlindExcess;
	// Public key corresponding to private nonce
	PublicKey m_publicNonce;
	// Public partial signature
	std::optional<Signature> m_partialSignatureOpt;
	// A message for other participants
	std::optional<std::string> m_messageOpt;
	// Signature, created with private key corresponding to 'public_blind_excess'
	std::optional<Signature> m_messageSignatureOpt;
};