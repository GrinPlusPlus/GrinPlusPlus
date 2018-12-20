#pragma once

#include <Config/Config.h>

// Forward Declarations
class IDatabase;
class IBlockChainServer;
class IP2PServer;

class RestServer
{
public:
	RestServer(const Config& config, IDatabase* pDatabase, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer);

	bool Start();
	bool Shutdown();

private:
	const Config& m_config;
	IDatabase* m_pDatabase;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;

	/* Server context handle */
	struct mg_context *ctx;
};