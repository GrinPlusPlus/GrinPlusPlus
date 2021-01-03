#pragma once

#include "NodeContext.h"
#include <Core/Config.h>
#include <API/Node/NodeServer.h>

class NodeRestServer
{
public:
	using UPtr = std::unique_ptr<NodeRestServer>;

	NodeRestServer(const std::shared_ptr<NodeContext>& pNodeContext, NodeServer::UPtr&& pNodeServer)
		: m_pNodeContext(pNodeContext), m_pNodeServer(std::move(pNodeServer)) { }
	~NodeRestServer() = default;

	static NodeRestServer::UPtr Create(const Config& config, std::shared_ptr<NodeContext> pNodeContext);

private:
	std::shared_ptr<NodeContext> m_pNodeContext;
	NodeServer::UPtr m_pNodeServer;
};