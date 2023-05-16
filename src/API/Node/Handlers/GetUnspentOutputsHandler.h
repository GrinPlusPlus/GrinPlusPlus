#pragma once

#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class GetUnspentOutputsHandler : public RPCMethod
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