#pragma once

#include "NodeClients/DefaultNodeClient.h"

#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <memory>

// Forward Declarations
class NodeRestServer;

class NodeDaemon
{
public:
	static std::shared_ptr<NodeDaemon> Create(const Config& config);
	~NodeDaemon();

	INodeClientPtr GetNodeClient() { return m_pNodeClient; }

	void UpdateDisplay(const int secondsRunning);

private:
	NodeDaemon(const Config& config, std::shared_ptr<NodeRestServer> pNodeRestServer, std::shared_ptr<DefaultNodeClient> pNodeClient);

	const Config& m_config;

	std::shared_ptr<NodeRestServer> m_pNodeRestServer;
	std::shared_ptr<DefaultNodeClient> m_pNodeClient;
};