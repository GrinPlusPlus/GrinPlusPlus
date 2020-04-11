#pragma once

#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>

class BuildCoinbaseHandler : RPCMethod
{
public:
	BuildCoinbaseHandler() = default;
	virtual ~BuildCoinbaseHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		return request.BuildError(RPC::ErrorCode::METHOD_NOT_FOUND, "Not implemented");
	}
};