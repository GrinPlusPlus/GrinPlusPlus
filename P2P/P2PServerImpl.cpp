#include "P2PServerImpl.h"

#include <BlockChainServer.h>

P2PServer::P2PServer(const Config& config, IBlockChainServer& blockChainServer, IDatabase& database)
	: m_config(config), 
	m_blockChainServer(blockChainServer), 
	m_database(database), 
	m_peerManager(config, database.GetPeerDB()), 
	m_connectionManager(config, m_peerManager, blockChainServer)
{

}

void P2PServer::StartServer()
{
	// Start ConnectionManager
	m_connectionManager.Start();
}

void P2PServer::StopServer()
{
	m_connectionManager.Stop();
}

size_t P2PServer::GetNumberOfConnectedPeers() const
{
	return m_connectionManager.GetNumberOfActiveConnections();
}

const SyncStatus& P2PServer::GetSyncStatus() const
{
	return m_connectionManager.GetSyncStatus();
}

namespace P2PAPI
{
	EXPORT IP2PServer* StartP2PServer(const Config& config, IBlockChainServer& blockChainServer, IDatabase& database)
	{
		P2PServer* pServer = new P2PServer(config, blockChainServer, database);
		pServer->StartServer();

		return pServer;
	}

	EXPORT void ShutdownP2PServer(IP2PServer* pP2PServer)
	{
		P2PServer* pServer = (P2PServer*)pP2PServer;
		pServer->StopServer();
		
		delete pServer;
		pServer = nullptr;
	}
}