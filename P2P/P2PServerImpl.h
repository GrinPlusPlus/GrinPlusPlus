#pragma once

#include "ConnectionManager.h"
#include "Seed/PeerManager.h"

#include <P2PServer.h>
#include <Database/Database.h>
#include <Config/Config.h>
#include <TxPool/TransactionPool.h>

class P2PServer : public IP2PServer
{
public:
	P2PServer(const Config& config, IBlockChainServer& blockChainServer, IDatabase& database, ITransactionPool& transactionPool);
	virtual ~P2PServer() = default;

	void StartServer();
	void StopServer();

	virtual size_t GetNumberOfConnectedPeers() const override final;
	virtual const SyncStatus& GetSyncStatus() const override final;

private:
	PeerManager m_peerManager;
	ConnectionManager m_connectionManager;
	const Config m_config;
	IBlockChainServer& m_blockChainServer;
	IDatabase& m_database;
};