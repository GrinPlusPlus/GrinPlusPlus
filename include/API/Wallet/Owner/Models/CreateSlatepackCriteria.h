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
#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/DTOs/PostMethodDTO.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <optional>

#include <optional>
#include <string>

class CreateSlatepackCriteria
{
public:
	CreateSlatepackCriteria(
		const SessionToken& token,
		const Slate slate,
		const std::optional<std::string>& addressOpt
	) : m_token(token), m_slate(slate), m_addressOpt(addressOpt) { }

	static CreateSlatepackCriteria FromJSON(const Json::Value& paramsJson)
	{
		if (paramsJson.isObject())
		{
			SessionToken token = SessionToken::FromBase64(
				JsonUtil::GetRequiredString(paramsJson, "session_token")
			);

			const Json::Value& slateJson = paramsJson["slate"];
			if (slateJson == Json::nullValue)
			{
				throw API_EXCEPTION(
					RPC::Errors::TX_ID_MISSING.GetCode(),
					"'slate' missing"
				);
			}

			const Slate slate = Slate::FromJSON(slateJson);
			const std::optional<std::string> addressStrOpt = JsonUtil::GetStringOpt(slateJson, "address");

			return CreateSlatepackCriteria(token, slate, addressStrOpt);
		}

		throw API_EXCEPTION(
			RPC::ErrorCode::INVALID_PARAMS,
			"Expected object with 3 or more parameters (required: session_token, tx_id, method)"
		);
	}

	const std::optional<std::string>& GetAddress() const { return m_addressOpt; }
	Slate GetSlate() const noexcept { return m_slate; }
	const SessionToken& GetToken() const noexcept { return m_token; }

private:
	SessionToken m_token;
	Slate m_slate;
	std::optional<std::string> m_addressOpt;
};