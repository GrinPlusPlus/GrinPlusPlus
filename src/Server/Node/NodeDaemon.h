#pragma once

#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <memory>

// Forward Declarations
class IDatabase;
class IBlockChainServer;
class IP2PServer;
class NodeRestServer;
class TxHashSetManager;
class ITransactionPool;
class DefaultNodeClient;

class NodeDaemon
{
public:
	NodeDaemon(const Config& config);

	std::shared_ptr<INodeClient> Initialize();
	void UpdateDisplay(const int secondsRunning);
	void Shutdown();

private:
	const Config& m_config;

	NodeRestServer* m_pNodeRestServer;
	std::shared_ptr<DefaultNodeClient> m_pNodeClient;
};