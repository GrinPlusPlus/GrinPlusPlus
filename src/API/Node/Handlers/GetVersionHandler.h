#pragma once

#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <GrinVersion.h>
#include <optional>

class GetVersionHandler : public RPCMethod
{
public:
	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value json;
		json["node_version"] = GRINPP_VERSION;
		
		Json::Value result;
		result["Ok"] = json;
		
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};