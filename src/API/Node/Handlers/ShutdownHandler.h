#pragma once

#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <GrinVersion.h>
#include <optional>

class ShutdownHandler : public RPCMethod
{
public:
	RPC::Response Handle(const RPC::Request& request) const final
	{
		Global::Shutdown();
		
		Json::Value result;
		result["Ok"] = "";
		
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};