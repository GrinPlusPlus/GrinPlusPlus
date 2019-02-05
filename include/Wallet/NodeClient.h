#pragma once

#include <stdint.h>

//
// INodeClient is an interface whose implementations communicate with nodes in various ways.
// For example, an HTTPNodeClient will connect to a node using HTTP and the node's REST APIs.
//
class INodeClient
{
public:
	virtual uint64_t GetChainHeight() const = 0;
};