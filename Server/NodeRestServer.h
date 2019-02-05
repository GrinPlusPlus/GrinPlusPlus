#pragma once

#include "ServerContainer.h"

#include <Config/Config.h>

// Forward Declarations
class IDatabase;
class IBlockChainServer;
class IP2PServer;

class NodeRestServer
{
public:
	NodeRestServer(const Config& config, IDatabase* pDatabase, TxHashSetManager* pTxHashSetManager, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer);

	bool Start();
	bool Shutdown();

private:
	const Config& m_config;
	IDatabase* m_pDatabase;
	TxHashSetManager* m_pTxHashSetManager;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;

	/* Server context handle */
	struct mg_context *ctx;

	ServerContainer m_serverContainer;
};