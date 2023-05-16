#pragma once

#include <Consensus.h>
#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class GetPeersHandler : public RPCMethod
{
public:
	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value result;
		result["Ok"] = "";
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};