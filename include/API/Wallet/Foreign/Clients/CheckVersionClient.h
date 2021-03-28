#pragma once

#include <Core/Util/JsonUtil.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorConnection.h>
#include <Wallet/Models/Slate/Slate.h>
#include <optional>
#include <cstdint>

class CheckVersionClient
{
public:
	struct Response
	{
		std::string error;
		uint16_t slate_version;
	};

	static Response Call(ITorConnection& connection) noexcept
	{
		try
		{
			RPC::Request checkVersionRequest = RPC::Request::BuildRequest("check_version");
			const RPC::Response response = connection.Invoke(checkVersionRequest, "/v2/foreign");
			if (response.GetError().has_value()) {
				LOG_ERROR_F("check_version failed with error: {}", response.GetError().value().GetMsg());
				return Response{ "check_version call failed", 0 };
			}

			const Json::Value responseJSON = response.GetResult().value();
			const Json::Value okJSON = JsonUtil::GetRequiredField(responseJSON, "Ok");
			const uint64_t apiVersion = JsonUtil::GetRequiredUInt64(okJSON, "foreign_api_version");
			if (apiVersion != 2) {
				return Response{ "foreign_api_version 2 not supported", 0 };
			}

			uint16_t highestVersion = 0;
			const Json::Value slateVersionsJSON = JsonUtil::GetRequiredField(okJSON, "supported_slate_versions");
			for (const Json::Value& slateVersion : slateVersionsJSON)
			{
				std::string versionStr(slateVersion.asString());
				if (!versionStr.empty() && versionStr[0] == 'V')
				{
					uint16_t version = (uint16_t)std::stoul(versionStr.substr(1));
					if (version > highestVersion && version <= MAX_SLATE_VERSION)
					{
						highestVersion = version;
					}
				}
			}

			return Response{ "", highestVersion };
		}
		catch (const std::exception& e)
		{
			LOG_ERROR_F("Exception thrown: {}", e.what());
			return Response{ "Error occurred during check_version.", 0 };
		}
	}
};