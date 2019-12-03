#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Slate.h>
#include <Wallet/Models/DTOs/PostMethodDTO.h>

class FinalizeCriteria
{
public:
	FinalizeCriteria(
		SessionToken&& token,
		Slate&& slate,
		const std::optional<std::string>& filePathOpt,
		const std::optional<PostMethodDTO>& postMethodOpt
	)
		: m_token(std::move(token)),
		m_slate(std::move(slate)),
		m_filePathOpt(filePathOpt),
		m_postMethodOpt(postMethodOpt)
	{

	}

	static FinalizeCriteria FromJSON(const Json::Value& json)
	{
		SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		Slate slate = Slate::FromJSON(JsonUtil::GetRequiredField(json, "slate"));
		const std::optional<std::string> filePathOpt = JsonUtil::GetStringOpt(json, "file");

		std::optional<PostMethodDTO> postMethodOpt = std::nullopt;
		std::optional<Json::Value> postJSON = JsonUtil::GetOptionalField(json, "post_tx");
		if (postJSON.has_value())
		{
			postMethodOpt = std::make_optional<PostMethodDTO>(PostMethodDTO::FromJSON(postJSON.value()));
		}

		return FinalizeCriteria(std::move(token), std::move(slate), filePathOpt, postMethodOpt);
	}

	const SessionToken& GetToken() const { return m_token; }
	const Slate& GetSlate() const { return m_slate; }
	const std::optional<std::string>& GetFile() const { return m_filePathOpt; }
	const std::optional<PostMethodDTO>& GetPostMethod() const { return m_postMethodOpt; }

private:
	SessionToken m_token;
	Slate m_slate;
	std::optional<std::string> m_filePathOpt;
	std::optional<PostMethodDTO> m_postMethodOpt;
};