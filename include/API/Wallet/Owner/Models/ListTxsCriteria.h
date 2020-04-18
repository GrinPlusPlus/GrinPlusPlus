#pragma once

#include <Core/Util/JsonUtil.h>
#include <Common/Util/TimeUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/WalletTxType.h>
#include <Core/Exceptions/APIException.h>
#include <optional>
#include <chrono>

class ListTxsCriteria
{
public:
	ListTxsCriteria(const SessionToken& token)
		: m_token(token)
	{

	}

	static ListTxsCriteria FromJSON(const Json::Value& json)
	{
		SessionToken token = SessionToken::FromBase64(
			JsonUtil::GetRequiredString(json, "session_token")
		);

		std::optional<std::chrono::system_clock::time_point> start_range = std::nullopt;
		if (json.isMember("start_range_ms"))
		{
			auto start = TimeUtil::ToTimePoint(json["start_range_ms"].asUInt());
			start_range = std::make_optional<std::chrono::system_clock::time_point>(std::move(start));
		}

		std::optional<std::chrono::system_clock::time_point> end_range = std::nullopt;
		if (json.isMember("end_range_ms"))
		{
			auto end = TimeUtil::ToTimePoint(json["end_range_ms"].asUInt());
			end_range = std::make_optional<std::chrono::system_clock::time_point>(std::move(end));
		}

		std::vector<EWalletTxType> statuses;
		if (json.isMember("statuses"))
		{
			const Json::Value& statusesJson = json["statuses"];
			if (!statusesJson.isArray())
			{
				throw API_EXCEPTION("'statuses' field must be an array");
			}

			for (const Json::Value& statusJson : statusesJson)
			{
				statuses.push_back(WalletTxType::FromKey(statusJson.asString()));
			}
		}

		return ListTxsCriteria(std::move(token));
	}

	const SessionToken& GetToken() const noexcept { return m_token; }
	const std::optional<std::chrono::system_clock::time_point>& GetStartRange() const noexcept { return m_startRange; }
	const std::optional<std::chrono::system_clock::time_point>& GetEndRange() const noexcept { return m_endRange; }
	const std::vector<EWalletTxType>& GetStatuses() const noexcept { return m_statuses; }

private:
	SessionToken m_token;
	std::optional<std::chrono::system_clock::time_point> m_startRange;
	std::optional<std::chrono::system_clock::time_point> m_endRange;
	std::vector<EWalletTxType> m_statuses;
};