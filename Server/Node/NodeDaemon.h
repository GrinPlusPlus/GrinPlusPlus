#pragma once

#include <Config/Config.h>
#include <Wallet/NodeClient.h>

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

	INodeClient* Initialize();
	void UpdateDisplay(const int secondsRunning);
	void Shutdown();

private:
	const Config& m_config;
	IDatabase* m_pDatabase;
	TxHashSetManager* m_pTxHashSetManager;
	ITransactionPool* m_pTransactionPool;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;

	NodeRestServer* m_pNodeRestServer;
	DefaultNodeClient* m_pNodeClient;
};