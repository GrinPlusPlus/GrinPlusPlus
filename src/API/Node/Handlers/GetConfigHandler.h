#pragma once

#include <Core/Config.h>
#include <Core/Global.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class GetConfigHandler : public RPCMethod
{
public:
	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value result;
		const Config& config = Global::GetConfig();

		Json::Value config_json;
		config_json["min_peers"] = config.GetMinPeers();
		config_json["max_peers"] = config.GetMaxPeers();
		config_json["min_confirmations"] = config.GetMinimumConfirmations();

		result["Ok"] = config_json;

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};