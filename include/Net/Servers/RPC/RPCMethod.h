#pragma once

#include <Net/Clients/RPC/RPC.h>

class RPCMethod
{
public:
	virtual ~RPCMethod() = default;

	virtual RPC::Response Handle(const RPC::Request& request) const = 0;
};