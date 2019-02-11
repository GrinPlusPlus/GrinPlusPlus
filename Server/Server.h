#pragma once

#include <Config/Config.h>

// Forward Declarations
class IDatabase;
class IBlockChainServer;
class IP2PServer;
class RestServer;
class TxHashSetManager;
class ITransactionPool;

class Server
{
public:
	Server();

	void Run();

private:
	Config m_config;
	IDatabase* m_pDatabase;
	TxHashSetManager* m_pTxHashSetManager;
	ITransactionPool* m_pTransactionPool;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;

	RestServer* m_pRestServer;
};