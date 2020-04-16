#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/KeyChainPath.h>
#include <optional>

class BuildCoinbaseCriteria
{
public:
	BuildCoinbaseCriteria(
		const SessionToken& token,
		const uint64_t fees,
		const std::optional<KeyChainPath>& keyId
	) : m_token(token), m_fees(fees), m_keyId(keyId) { }

	static BuildCoinbaseCriteria FromJSON(const Json::Value& json)
	{
		SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		const uint64_t fees = JsonUtil::GetRequiredUInt64(json, "fees");

		std::optional<std::string> keyIdOpt = JsonUtil::GetStringOpt(json, "key_id");
		std::optional<KeyChainPath> keyPath = std::nullopt;
		if (keyIdOpt.has_value())
		{
			keyPath = std::make_optional<KeyChainPath>(KeyChainPath::FromHex(keyIdOpt.value()));
		}

		return BuildCoinbaseCriteria(token, fees, keyPath);
	}

	const SessionToken& GetToken() const noexcept { return m_token; }
	uint64_t GetFees() const noexcept { return m_fees; }
	const std::optional<KeyChainPath>& GetPath() const noexcept { return m_keyId; }

private:
	SessionToken m_token;
	uint64_t m_fees;
	std::optional<KeyChainPath> m_keyId;
};