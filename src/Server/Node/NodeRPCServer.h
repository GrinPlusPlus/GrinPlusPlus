#pragma once

#include "NodeContext.h"
#include <Core/Config.h>
#include <API/Node/NodeServer.h>

class NodeRPCServer
{
public:
	using UPtr = std::unique_ptr<NodeRPCServer>;

	NodeRPCServer(const std::shared_ptr<NodeContext>& pNodeContext, NodeServer::UPtr&& pNodeServer)
		: m_pNodeContext(pNodeContext), m_pNodeServer(std::move(pNodeServer)) { }
	~NodeRPCServer() = default;

	static NodeRPCServer::UPtr Create(const ServerPtr& pServer, std::shared_ptr<NodeContext> pNodeContext);

private:
	std::shared_ptr<NodeContext> m_pNodeContext;
	NodeServer::UPtr m_pNodeServer;
};