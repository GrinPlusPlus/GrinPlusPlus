#pragma once

#include "NodeClients/DefaultNodeClient.h"
#include "..//GrinJoin/GrinJoinController.h"

#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <memory>

// Forward Declarations
class NodeRestServer;

class NodeDaemon
{
public:
	static std::shared_ptr<NodeDaemon> Create(const Config& config);
	~NodeDaemon() = default;

	INodeClientPtr GetNodeClient() { return m_pNodeClient; }

	void UpdateDisplay(const int secondsRunning);

private:
	NodeDaemon(const Config& config, std::shared_ptr<NodeRestServer> pNodeRestServer, std::shared_ptr<DefaultNodeClient> pNodeClient, std::shared_ptr<GrinJoinController> pGrinJoinController);

	const Config& m_config;

	std::shared_ptr<NodeRestServer> m_pNodeRestServer;
	std::shared_ptr<DefaultNodeClient> m_pNodeClient;
	std::shared_ptr<GrinJoinController> m_pGrinJoinController;
};