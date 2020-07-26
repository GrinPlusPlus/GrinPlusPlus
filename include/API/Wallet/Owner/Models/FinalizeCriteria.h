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

		Slate slate;
		std::optional<SlatepackMessage> slatepack_opt;

		auto slatepack_str_opt = JsonUtil::GetStringOpt(json, "slatepack");
		if (slatepack_str_opt.has_value()) {
			SlatepackMessage slatepack = decryptor.Decrypt(token, slatepack_str_opt.value());

			ByteBuffer buffer{ slatepack.m_payload };
			slate = Slate::Deserialize(buffer);
			slatepack_opt = std::make_optional(std::move(slatepack));
		} else {
			slate = Slate::FromJSON(JsonUtil::GetRequiredField(json, "slate"));
		}

		std::optional<PostMethodDTO> postMethodOpt = std::nullopt;
		std::optional<Json::Value> postJSON = JsonUtil::GetOptionalField(json, "post_tx");
		if (postJSON.has_value())
		{
			postMethodOpt = std::make_optional<PostMethodDTO>(PostMethodDTO::FromJSON(postJSON.value()));
		}

		return FinalizeCriteria(
			token,
			std::move(slatepack_opt),
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