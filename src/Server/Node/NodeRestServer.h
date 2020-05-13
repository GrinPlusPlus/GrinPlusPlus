#pragma once

#include "NodeContext.h"
#include <Config/Config.h>
#include <Net/Servers/RPC/RPCServer.h>

class NodeRestServer
{
public:
	using UPtr = std::unique_ptr<NodeRestServer>;

	NodeRestServer(const std::shared_ptr<NodeContext>& pNodeContext, const RPCServerPtr& pRPCServer)
		: m_pNodeContext(pNodeContext), m_pRPCServer(pRPCServer) { }
	~NodeRestServer() = default;

	static NodeRestServer::UPtr Create(const Config& config, std::shared_ptr<NodeContext> pNodeContext);

private:
	std::shared_ptr<NodeContext> m_pNodeContext;
	RPCServerPtr m_pRPCServer;
};