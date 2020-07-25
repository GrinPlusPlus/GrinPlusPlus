#pragma once

#include <Wallet/Enums/PostMethod.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Tor/TorAddressParser.h>
#include <optional>
#include <string>

class PostMethodDTO
{
public:
	PostMethodDTO(const EPostMethod& method, const std::optional<TorAddress>& grinjoinAddressOpt)
		: m_method(method), m_grinjoinAddressOpt(grinjoinAddressOpt) { }

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["method"] = PostMethod::ToString(m_method);

		if (m_grinjoinAddressOpt.has_value()) {
			json["grinjoin_address"] = m_grinjoinAddressOpt.value().ToString();
		}

		return json;
	}

	static PostMethodDTO FromJSON(const Json::Value& postMethodJSON)
	{
		const std::string method = JsonUtil::GetRequiredString(postMethodJSON, "method");

		const std::optional<std::string> grinjoinAddressOpt = JsonUtil::GetStringOpt(postMethodJSON, "grinjoin_address");
		std::optional<TorAddress> torAddress = grinjoinAddressOpt.has_value() ? TorAddressParser::Parse(grinjoinAddressOpt.value()) : std::nullopt;

		return PostMethodDTO(PostMethod::FromString(method), torAddress);
	}

	EPostMethod GetMethod() const { return m_method; }
	const std::optional<TorAddress>& GetGrinJoinAddress() const { return m_grinjoinAddressOpt; }

private:
	EPostMethod m_method;
	std::optional<TorAddress> m_grinjoinAddressOpt;
};