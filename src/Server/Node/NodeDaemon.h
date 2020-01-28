#pragma once

#include "NodeClients/DefaultNodeClient.h"
#include "..//GrinJoin/GrinJoinController.h"

#include <Core/Context.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <memory>

// Forward Declarations
class NodeRestServer;

class NodeDaemon
{
public:
	static std::shared_ptr<NodeDaemon> Create(const Context::Ptr& pContext);

	NodeDaemon(
		const Context::Ptr& pContext,
		std::shared_ptr<NodeRestServer> pNodeRestServer,
		std::shared_ptr<DefaultNodeClient> pNodeClient,
		std::shared_ptr<GrinJoinController> pGrinJoinController
	);
	~NodeDaemon() = default;

	INodeClientPtr GetNodeClient() { return m_pNodeClient; }

	void UpdateDisplay(const int secondsRunning);

private:

	Context::Ptr m_pContext;
	std::shared_ptr<NodeRestServer> m_pNodeRestServer;
	std::shared_ptr<DefaultNodeClient> m_pNodeClient;
	std::shared_ptr<GrinJoinController> m_pGrinJoinController;
};