#include "P2PServerImpl.h"

#include <BlockChainServer.h>

P2PServer::P2PServer(const Config& config, IBlockChainServer& blockChainServer, IDatabase& database, ITransactionPool& transactionPool)
	: m_config(config), 
	m_blockChainServer(blockChainServer), 
	m_database(database), 
	m_peerManager(config, database.GetPeerDB()), 
	m_connectionManager(config, m_peerManager, blockChainServer, transactionPool)
{

}

void P2PServer::StartServer()
{
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
	EXPORT IP2PServer* StartP2PServer(const Config& config, IBlockChainServer& blockChainServer, IDatabase& database, ITransactionPool& transactionPool)
	{
		P2PServer* pServer = new P2PServer(config, blockChainServer, database, transactionPool);
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