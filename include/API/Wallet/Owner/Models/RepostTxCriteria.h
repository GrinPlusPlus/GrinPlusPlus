#pragma once

#include <Wallet/Enums/PostMethod.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Tor/TorAddressParser.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/SessionToken.h>
#include <optional>
#include <string>

class RepostTxCriteria
{
public:
	RepostTxCriteria(
		const uint32_t txId,
		const SessionToken& token,
		const EPostMethod& method,
		const std::optional<TorAddress>& grinjoinAddressOpt
	) : m_txId(txId), m_token(token), m_method(method), m_grinjoinAddressOpt(grinjoinAddressOpt) { }

	static RepostTxCriteria FromJSON(const Json::Value& paramsJson)
	{
		if (paramsJson.isObject())
		{
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

			const std::string method = JsonUtil::GetRequiredString(paramsJson, "method");

			const std::optional<std::string> grinjoinAddressOpt = JsonUtil::GetStringOpt(paramsJson, "grinjoin_address");
			std::optional<TorAddress> torAddress = grinjoinAddressOpt.has_value() ? TorAddressParser::Parse(grinjoinAddressOpt.value()) : std::nullopt;

			return RepostTxCriteria(tx_id, token, PostMethod::FromString(method), torAddress);
		}

		throw API_EXCEPTION(
			RPC::ErrorCode::INVALID_PARAMS,
			"Expected object with 3 or more parameters (required: session_token, tx_id, method)"
		);
	}

	uint32_t GetTxId() const noexcept { return m_txId; }
	const SessionToken& GetToken() const noexcept { return m_token; }
	EPostMethod GetMethod() const noexcept { return m_method; }
	const std::optional<TorAddress>& GetGrinJoinAddress() const noexcept { return m_grinjoinAddressOpt; }

private:
	uint32_t m_txId;
	SessionToken m_token;
	EPostMethod m_method;
	std::optional<TorAddress> m_grinjoinAddressOpt;
};