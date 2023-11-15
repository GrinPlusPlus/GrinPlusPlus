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
		const Config& config = Global::GetConfig();

		Json::Value config_json;
		config_json["min_peers"] = config.GetMinPeers();
		config_json["max_peers"] = config.GetMaxPeers();
		config_json["min_confirmations"] = config.GetMinimumConfirmations();
		config_json["address_reuse"] = config.ShouldReuseAddresses();
		
		Json::Value preferred_peers;
		for (const IPAddress& peer : config.GetPreferredPeers())
		{
			preferred_peers.append(peer.Format());
		}
		config_json["preferred_peers"] = preferred_peers;
		
		Json::Value allowed_peers;
		for (const IPAddress& peer : config.GetAllowedPeers())
		{
			allowed_peers.append(peer.Format());
		}
		config_json["allowed_peers"] = allowed_peers;
		
		Json::Value blocked_peers;
		for (const IPAddress& peer : config.GetBlockedPeers())
		{
			blocked_peers.append(peer.Format());
		}
		config_json["blocked_peers"] = blocked_peers;
		
		Json::Value result;
		result["Ok"] = config_json;
		return request.BuildResult(result);

		return request.BuildResult(config_json);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};