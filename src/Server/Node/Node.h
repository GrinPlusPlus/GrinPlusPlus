#pragma once

#include "NodeRestServer.h"

#include <Core/Config.h>
#include <Wallet/NodeClient.h>
#include <memory>

// Forward Declarations
class Context;
class NodeRestServer;
class DefaultNodeClient;

class Node
{
public:
	static std::unique_ptr<Node> Create(const std::shared_ptr<Context>& pContext);

	Node(
		const std::shared_ptr<Context>& pContext,
		std::unique_ptr<NodeRestServer>&& pNodeRestServer,
		std::shared_ptr<DefaultNodeClient> pNodeClient
	);
	~Node();

	std::shared_ptr<INodeClient> GetNodeClient() const;

	void UpdateDisplay(const int secondsRunning);

private:
	std::shared_ptr<Context> m_pContext;
	std::unique_ptr<NodeRestServer> m_pNodeRestServer;
	std::shared_ptr<DefaultNodeClient> m_pNodeClient;
};