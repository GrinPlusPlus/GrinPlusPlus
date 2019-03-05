#pragma once

#include <Config/Config.h>

// Forward Declarations
class IDatabase;
class IBlockChainServer;
class IP2PServer;
class TxHashSetManager;
struct NodeContext;
struct mg_context;

class NodeRestServer
{
public:
	NodeRestServer(const Config& config, IDatabase* pDatabase, TxHashSetManager* pTxHashSetManager, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer);
	~NodeRestServer();

	bool Initialize();
	bool Shutdown();

private:
	const Config& m_config;
	IDatabase* m_pDatabase;
	TxHashSetManager* m_pTxHashSetManager;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;

	NodeContext* m_pNodeContext;
	mg_context* m_pNodeCivetContext;
};