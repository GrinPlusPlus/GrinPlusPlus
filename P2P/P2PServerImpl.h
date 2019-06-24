#pragma once

#include "ConnectionManager.h"
#include "Seed/PeerManager.h"

#include <P2P/P2PServer.h>
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

	virtual const SyncStatus& GetSyncStatus() const override final;

	virtual std::pair<size_t, size_t> GetNumberOfConnectedPeers() const override final;
	virtual std::vector<Peer> GetAllPeers() const override final;
	virtual std::vector<ConnectedPeer> GetConnectedPeers() const override final;

	virtual std::optional<Peer> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const override final;
	virtual bool BanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt, const EBanReason banReason) override final;
	virtual bool UnbanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) override final;
	virtual bool UnbanAllPeers() override final;

private:
	PeerManager m_peerManager;
	ConnectionManager m_connectionManager;
	const Config m_config;
	IBlockChainServer& m_blockChainServer;
	IDatabase& m_database;
};