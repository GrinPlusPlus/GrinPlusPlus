#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/Slate/Slate.h>

class ReceiveCriteria
{
public:
	ReceiveCriteria(
		SessionToken&& token,
		Slate&& slate,
		const std::optional<std::string>& messageOpt,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& filePathOpt
	)
		: m_token(std::move(token)),
		m_slate(std::move(slate)),
		m_messageOpt(messageOpt),
		m_addressOpt(addressOpt),
		m_filePathOpt(filePathOpt)
	{

	}

	static ReceiveCriteria FromJSON(const Json::Value& json)
	{
		SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		Slate slate = Slate::FromJSON(JsonUtil::GetRequiredField(json, "slate"));

		const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");
		const std::optional<std::string> addressOpt = JsonUtil::GetStringOpt(json, "address");
		const std::optional<std::string> filePathOpt = JsonUtil::GetStringOpt(json, "file");

		return ReceiveCriteria(std::move(token), std::move(slate), messageOpt, addressOpt, filePathOpt);
	}

	const SessionToken& GetToken() const { return m_token; }
	const Slate& GetSlate() const { return m_slate; }
	const std::optional<std::string>& GetMsg() const { return m_messageOpt; }
	const std::optional<std::string>& GetAddress() const { return m_addressOpt; }
	const std::optional<std::string>& GetFile() const { return m_filePathOpt; }

private:
	SessionToken m_token;
	Slate m_slate;
	std::optional<std::string> m_messageOpt;
	std::optional<std::string> m_addressOpt;
	std::optional<std::string> m_filePathOpt;
};