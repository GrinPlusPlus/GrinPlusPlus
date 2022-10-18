#pragma once

#include <Core/Config.h>
#include <Core/Global.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Util/JsonUtil.h>

class GetTorConfigHandler : public RPCMethod
{
public:
	GetTorConfigHandler() { }
	virtual ~GetTorConfigHandler() = default;
		
	RPC::Response Handle(const RPC::Request& request) const final
	{
		Config& config = Global::GetConfig();
		
		Json::Value json;
		
		Json::Value bridges(Json::arrayValue);
		for (const std::string& bridge : config.GetTorBridgesList())
		{
			bridges.append(bridge);
		}

		json["torrc"] = config.GetTorrcFileContent();
		json["bridges"] = bridges;
		json["bridges_type"] = config.IsTorBridgesEnabled() ? config.IsSnowflakeEnabled() ? "snowflake" : "obfs4" : "";
		
		return request.BuildResult(json);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};