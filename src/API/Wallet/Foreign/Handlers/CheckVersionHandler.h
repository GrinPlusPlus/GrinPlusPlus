#pragma once

#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Foreign/Models/CheckVersionResponse.h>

class CheckVersionHandler : RPCMethod
{
public:
	CheckVersionHandler() = default;
	virtual ~CheckVersionHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		return request.BuildResult(CheckVersionResponse(2, { 4 }));
	}

	bool ContainsSecrets() const noexcept final { return false; }
};