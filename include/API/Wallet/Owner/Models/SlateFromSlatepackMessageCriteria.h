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

class SlateFromSlatepackMessageCriteria
{
public:
	SlateFromSlatepackMessageCriteria(
		const Slate slate,
		const SessionToken& token
	) : m_slate(slate), m_token(token) { }

	static SlateFromSlatepackMessageCriteria FromJSON(const Json::Value& paramsJson, const ISlatepackDecryptor& decryptor)
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
			ByteBuffer buffer{ slatepack.m_payload };
			Slate slate = Slate::Deserialize(buffer);

			return SlateFromSlatepackMessageCriteria(slate, token);
		}

		throw API_EXCEPTION(
			RPC::ErrorCode::INVALID_PARAMS,
			"Expected object with 3 or more parameters (required: session_token, tx_id, method)"
		);
	}

	Slate GetSlate() const noexcept { return m_slate; }
	const SessionToken& GetToken() const noexcept { return m_token; }

private:
	Slate m_slate;
	SessionToken m_token;
};