#pragma once

#include <Wallet/Enums/PostMethod.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Tor/TorAddressParser.h>
#include <Wallet/Models/Slatepack/SlatepackMessage.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Wallet/Models/Slate/Slate.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/SessionToken.h>
#include <API/Wallet/Owner/Utils/SlatepackDecryptor.h>

#include <optional>
#include <string>

class DecodeSlatepackCriteria
{
public:
	DecodeSlatepackCriteria(
		const SlatepackMessage slatepackMessage,
		const SessionToken& token
	) : m_slatepackMessage(slatepackMessage), m_token(token) { }

	static DecodeSlatepackCriteria FromJSON(const Json::Value& paramsJson, const ISlatepackDecryptor& decryptor)
	{
		if (paramsJson.isObject())
		{
			SessionToken token = SessionToken::FromBase64(
				JsonUtil::GetRequiredString(paramsJson, "session_token")
			);

			const Json::Value& messageJson = paramsJson["message"];
			if (messageJson == Json::nullValue)
			{
				throw API_EXCEPTION(
					RPC::Errors::TX_ID_MISSING.GetCode(),
					"'message' missing"
				);
			}

			const SlatepackMessage slatepack = decryptor.Decrypt(token, JsonUtil::GetRequiredString(messageJson, "slatepack"));

			return DecodeSlatepackCriteria(slatepack, token);
		}

		throw API_EXCEPTION(
			RPC::ErrorCode::INVALID_PARAMS,
			"Expected object with 3 or more parameters (required: session_token, tx_id, method)"
		);
	}

	SlatepackMessage GetSlatepackMessage() const noexcept { return m_slatepackMessage; }
	const SessionToken& GetToken() const noexcept { return m_token; }

private:
	SlatepackMessage m_slatepackMessage;
	SessionToken m_token;
};