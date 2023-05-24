#pragma once

#include <Wallet/Enums/PostMethod.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Tor/TorAddressParser.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/SessionToken.h>
#include <optional>
#include <string>


class GetStoredTxCriteria
{
public:
	GetStoredTxCriteria(
		const uuids::uuid txSlateId,
		const SessionToken& token
	) : m_txSlateId(txSlateId), m_token(token) { }

	static GetStoredTxCriteria FromJSON(const Json::Value& paramsJson)
	{
		if (paramsJson.isObject())
		{
			SessionToken token = SessionToken::FromBase64(
				JsonUtil::GetRequiredString(paramsJson, "session_token")
			);

			const Json::Value& txIdJson = paramsJson["slate_id"];
			if (txIdJson == Json::nullValue)
			{
				throw API_EXCEPTION(
					RPC::Errors::TX_ID_MISSING.GetCode(),
					"'slate_id' missing"
				);
			}

			const uuids::uuid slateId = uuids::uuid::from_string(JsonUtil::GetRequiredString(txIdJson,"slate_id")).value();

			return GetStoredTxCriteria(slateId, token);
		}

		throw API_EXCEPTION(
			RPC::ErrorCode::INVALID_PARAMS,
			"Expected object with 3 or more parameters (required: session_token, tx_id, method)"
		);
	}

	uuids::uuid GetTxSlateId() const noexcept { return m_txSlateId; }
	const SessionToken& GetToken() const noexcept { return m_token; }

private:
	uuids::uuid m_txSlateId;
	SessionToken m_token;
};