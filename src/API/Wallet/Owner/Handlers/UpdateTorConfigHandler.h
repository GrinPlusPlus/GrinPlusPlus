#pragma once

#include <Core/Config.h>
#include <Core/Global.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>
#include <fstream>

class UpdateTorConfigHandler : public RPCMethod
{
public:
	UpdateTorConfigHandler(const TorProcess::Ptr& pTorProcess)
		: m_pTorProcess(pTorProcess) { }
	virtual ~UpdateTorConfigHandler() = default;
		
	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params_json = request.GetParams().value();
		if (!params_json.isObject()) {
			return request.BuildError("INVALID_PARAMS", "Expected object parameter");
		}

		const Json::Value& json_params = request.GetParams().value();

		std::vector<Json::Value> torBridges = JsonUtil::GetRequiredArray(json_params, "bridges_list");
		bool enableSnowflake = JsonUtil::GetRequiredBool(json_params, "enable_snowflake");
		
		Config& config = Global::GetConfig();
		
		if (enableSnowflake)
		{
			if (!torBridges.empty())
			{
				return request.BuildError("INVALID_PARAMS", "Obfs4 bridges are not compatible with Snowflake");
			}
			config.EnableSnowflake();
		}
		else
		{
			config.DisableSnowflake();
		}
		
		if(!torBridges.empty())
		{
			for (const Json::Value& bridge : torBridges)
			{
				config.AddObfs4TorBridge(bridge.asString());
			}	
		}
		else
		{
			config.DisableObfsBridges();
		}

		Json::Value bridges(Json::arrayValue);
		for (const std::string& bridge : config.GetTorBridgesList())
		{
			bridges.append(bridge);
		}

		Json::Value result;
		result["status"] = "SUCCESS";

		result["bridges"] = bridges;
		result["bridges_type"] = config.IsTorBridgesEnabled() ? config.IsSnowflakeEnabled() ? "snowflake" : "obfs4" : "";
		
		Json::Value response_json;
		response_json["Ok"] = result;
		return request.BuildResult(response_json);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TorProcess::Ptr m_pTorProcess;
};