#pragma once

#include <Config/Config.h>

// Forward Declarations
class IDatabase;
class IBlockChainServer;
class IP2PServer;

class Server
{
public:
	Server();

	void Run();

private:
	Config m_config;
	IDatabase* m_pDatabase;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;
};