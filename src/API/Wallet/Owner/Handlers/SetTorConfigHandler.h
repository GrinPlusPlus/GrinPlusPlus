#pragma once

#include <Core/Config.h>
#include <Core/Global.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>
#include <fstream>

class SetTorConfigHandler : public RPCMethod
{
public:
	SetTorConfigHandler(const TorProcess::Ptr& pTorProcess)
		: m_pTorProcess(pTorProcess) { }
	virtual ~SetTorConfigHandler() = default;
		
	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params_json = request.GetParams().value();
		if (!params_json.isObject()) {
			return request.BuildError("INVALID_PARAMS", "Expected object parameter");
		}
		
		Config& config = Global::GetConfig();

		const Json::Value& json_params = request.GetParams().value();

		std::string tor_bridge = JsonUtil::GetRequiredString(json_params, "bridge");
		auto snowflake_enabled = JsonUtil::GetUInt32Opt(json_params, "snowflake");

		if (!tor_bridge.empty() && snowflake_enabled.has_value()) {
			return request.BuildError("INVALID_PARAMS", "Expected object parameter");
		}

		if (snowflake_enabled.has_value()) {
			if (snowflake_enabled.value() == 0)
			{
				config.DisableSnowflake();
			}
			else
			{
				config.EnableSnowflake();
			}
			
			m_pTorProcess->RetryInit();
		}

		if (!tor_bridge.empty()) {
			config.AddTorBridge(tor_bridge);
			m_pTorProcess->RetryInit();
		}
		
		Json::Value result;
		result["status"] = "SUCCESS";
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TorProcess::Ptr m_pTorProcess;
};