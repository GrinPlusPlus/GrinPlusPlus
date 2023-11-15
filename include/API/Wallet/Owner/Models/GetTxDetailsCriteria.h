#pragma once

#include <Wallet/Enums/PostMethod.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Tor/TorAddressParser.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/SessionToken.h>
#include <optional>
#include <string>


class GetTxDetailsCriteria
{
public:
	GetTxDetailsCriteria(
		const uint32_t txSlateId,
		const SessionToken& token
	) : m_txId(txSlateId), m_token(token) { }

	static GetTxDetailsCriteria FromJSON(const Json::Value& paramsJson)
	{
		if (!paramsJson.isObject())
		{
			throw API_EXCEPTION(
				RPC::ErrorCode::INVALID_PARAMS,
				"Expected object with 2 (session_token, slate_id)"
			);
		}

		SessionToken token = SessionToken::FromBase64(
			JsonUtil::GetRequiredString(paramsJson, "session_token")
		);

		const Json::Value& txIdJson = paramsJson["tx_id"];
		if (txIdJson == Json::nullValue)
		{
			throw API_EXCEPTION(
				RPC::Errors::TX_ID_MISSING.GetCode(),
				"'tx_id' missing"
			);
		}

		const uint32_t tx_id = (uint32_t)JsonUtil::ConvertToUInt64(txIdJson);

		return GetTxDetailsCriteria(tx_id, token);
	}

	uint32_t GetTxId() const noexcept { return m_txId; }
	const SessionToken& GetToken() const noexcept { return m_token; }

private:
	uint32_t m_txId;
	SessionToken m_token;
};