#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <API/Wallet/Owner/Utils/SlatepackDecryptor.h>

class ReceiveCriteria
{
public:
	ReceiveCriteria(
		SessionToken&& token,
		std::optional<SlatepackMessage>&& slatepackOpt,
		Slate&& slate,
		const std::optional<std::string>& messageOpt,
		const std::optional<SlatepackAddress>& addressOpt,
		const std::optional<std::string>& filePathOpt
	)
		: m_token(std::move(token)),
		m_slatepackOpt(std::move(slatepackOpt)),
		m_slate(std::move(slate)),
		m_messageOpt(messageOpt),
		m_addressOpt(addressOpt),
		m_filePathOpt(filePathOpt)
	{

	}

	static ReceiveCriteria FromJSON(const Json::Value& json, const ISlatepackDecryptor& decryptor)
	{
		SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		SlatepackMessage slatepack = decryptor.Decrypt(token, JsonUtil::GetRequiredString(json, "slatepack"));
		ByteBuffer buffer{ slatepack.m_payload };
		Slate slate = Slate::Deserialize(buffer);

		const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");
		std::optional<SlatepackAddress> addressOpt = slatepack.m_sender.IsNull() ? std::nullopt : std::make_optional(slatepack.m_sender);

		const std::optional<std::string> filePathOpt = JsonUtil::GetStringOpt(json, "file");

		return ReceiveCriteria(
			std::move(token),
			std::make_optional<SlatepackMessage>(std::move(slatepack)),
			std::move(slate),
			messageOpt,
			addressOpt,
			filePathOpt
		);
	}

	const SessionToken& GetToken() const noexcept { return m_token; }
	const std::optional<SlatepackMessage>& GetSlatepack() const noexcept { return m_slatepackOpt; }
	const Slate& GetSlate() const noexcept { return m_slate; }
	const std::optional<std::string>& GetMsg() const noexcept { return m_messageOpt; }
	const std::optional<SlatepackAddress>& GetAddress() const noexcept { return m_addressOpt; }
	const std::optional<std::string>& GetFile() const noexcept { return m_filePathOpt; }

private:
	SessionToken m_token;
	std::optional<SlatepackMessage> m_slatepackOpt;
	Slate m_slate;
	std::optional<std::string> m_messageOpt;
	std::optional<SlatepackAddress> m_addressOpt;
	std::optional<std::string> m_filePathOpt;
};