#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/Models/DTOs/PostMethodDTO.h>
#include <API/Wallet/Owner/Utils/SlatepackDecryptor.h>

class FinalizeCriteria
{
public:
	FinalizeCriteria(
		const SessionToken& token,
		std::optional<SlatepackMessage>&& slatepackOpt,
		Slate&& slate,
		const std::optional<PostMethodDTO>& postMethodOpt
	)
		: m_token(token),
		m_slatepackOpt(std::move(slatepackOpt)),
		m_slate(std::move(slate)),
		m_postMethodOpt(postMethodOpt) { }

	static FinalizeCriteria FromJSON(const Json::Value& json, const ISlatepackDecryptor& decryptor)
	{
		SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		SlatepackMessage slatepack = decryptor.Decrypt(token, JsonUtil::GetRequiredString(json, "slatepack"));

		ByteBuffer buffer{ slatepack.m_payload };
		Slate slate = Slate::Deserialize(buffer);

		std::optional<PostMethodDTO> postMethodOpt = std::nullopt;
		std::optional<Json::Value> postJSON = JsonUtil::GetOptionalField(json, "post_tx");
		if (postJSON.has_value())
		{
			postMethodOpt = std::make_optional<PostMethodDTO>(PostMethodDTO::FromJSON(postJSON.value()));
		}

		return FinalizeCriteria(
			token,
			std::make_optional<SlatepackMessage>(std::move(slatepack)),
			std::move(slate),
			postMethodOpt
		);
	}

	const SessionToken& GetToken() const noexcept { return m_token; }
	const std::optional<SlatepackMessage>& GetSlatepack() const noexcept { return m_slatepackOpt; }
	const Slate& GetSlate() const noexcept { return m_slate; }
	const std::optional<PostMethodDTO>& GetPostMethod() const noexcept { return m_postMethodOpt; }

private:
	SessionToken m_token;
	std::optional<SlatepackMessage> m_slatepackOpt;
	Slate m_slate;
	std::optional<PostMethodDTO> m_postMethodOpt;
};