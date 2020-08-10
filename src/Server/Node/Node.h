#pragma once

#include "NodeRestServer.h"
#include "NodeClients/DefaultNodeClient.h"

#include <Core/Context.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <memory>

// Forward Declarations
class NodeRestServer;

class Node
{
public:
	static std::unique_ptr<Node> Create(const Context::Ptr& pContext);

	Node(
		const Context::Ptr& pContext,
		std::unique_ptr<NodeRestServer>&& pNodeRestServer,
		std::shared_ptr<DefaultNodeClient> pNodeClient
	);
	~Node();

	INodeClientPtr GetNodeClient() { return m_pNodeClient; }

	void UpdateDisplay(const int secondsRunning);

private:
	Context::Ptr m_pContext;
	std::unique_ptr<NodeRestServer> m_pNodeRestServer;
	std::shared_ptr<DefaultNodeClient> m_pNodeClient;
};