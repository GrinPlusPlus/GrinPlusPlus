#pragma once

#include <Core/Util/JsonUtil.h>
#include <Common/Util/TimeUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/WalletTxType.h>
#include <Core/Exceptions/APIException.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <unordered_set>
#include <optional>
#include <chrono>

class ListTxsCriteria
{
public:
	ListTxsCriteria(
		const SessionToken& token,
		const std::unordered_set<uint32_t>& txIds,
		const std::optional<std::chrono::system_clock::time_point>& start_range,
		const std::optional<std::chrono::system_clock::time_point>& end_range,
		const std::unordered_set<EWalletTxType>& statuses
	) : m_token(token), m_txIds(txIds), m_startRange(start_range), m_endRange(end_range), m_statuses(statuses) { }

	static ListTxsCriteria FromJSON(const Json::Value& json)
	{
		SessionToken token = SessionToken::FromBase64(
			JsonUtil::GetRequiredString(json, "session_token")
		);


		std::unordered_set<uint32_t> ids;
		std::vector<Json::Value> ids_json = JsonUtil::GetArray(json, "ids");
		std::transform(
			ids_json.cbegin(), ids_json.cend(),
			std::inserter(ids, ids.end()),
			[](const Json::Value& id_json) { return (uint32_t)JsonUtil::ConvertToUInt64(id_json); }
		);

		std::optional<std::chrono::system_clock::time_point> start_range = std::nullopt;
		if (json.isMember("start_range_ms"))
		{
			auto start = TimeUtil::ToTimePoint(JsonUtil::ConvertToUInt64(json["start_range_ms"]));
			start_range = std::make_optional<std::chrono::system_clock::time_point>(std::move(start));
		}

		std::optional<std::chrono::system_clock::time_point> end_range = std::nullopt;
		if (json.isMember("end_range_ms"))
		{
			auto end = TimeUtil::ToTimePoint(JsonUtil::ConvertToUInt64(json["end_range_ms"]));
			end_range = std::make_optional<std::chrono::system_clock::time_point>(std::move(end));
		}

		std::unordered_set<EWalletTxType> statuses;
		if (json.isMember("statuses"))
		{
			const Json::Value& statusesJson = json["statuses"];
			if (!statusesJson.isArray())
			{
				throw API_EXCEPTION(
					RPC::Errors::INVALID_STATUSES.GetCode(),
					"'statuses' field must be an array"
				);
			}

			for (const Json::Value& statusJson : statusesJson)
			{
				try
				{
					statuses.insert(WalletTxType::FromKey(statusJson.asString()));
				}
				catch (std::exception&)
				{
					std::string status = statusJson.asString();
					throw API_EXCEPTION_F(
						RPC::Errors::INVALID_STATUSES.GetCode(),
						"{} is not a valid status",
						status
					);
				}
			}
		}

		return ListTxsCriteria(token, ids, start_range, end_range, statuses);
	}

	const SessionToken& GetToken() const noexcept { return m_token; }
	const std::unordered_set<uint32_t>& GetTxIds() const noexcept { return m_txIds; }
	const std::optional<std::chrono::system_clock::time_point>& GetStartRange() const noexcept { return m_startRange; }
	const std::optional<std::chrono::system_clock::time_point>& GetEndRange() const noexcept { return m_endRange; }
	const std::unordered_set<EWalletTxType>& GetStatuses() const noexcept { return m_statuses; }

private:
	SessionToken m_token;
	std::unordered_set<uint32_t> m_txIds;
	std::optional<std::chrono::system_clock::time_point> m_startRange;
	std::optional<std::chrono::system_clock::time_point> m_endRange;
	std::unordered_set<EWalletTxType> m_statuses;
};