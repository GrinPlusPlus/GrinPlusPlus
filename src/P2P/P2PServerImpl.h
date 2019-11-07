#pragma once

#include "Dandelion.h"
#include "ConnectionManager.h"
#include "Pipeline/Pipeline.h"
#include "Sync/Syncer.h"
#include "Seed/Seeder.h"
#include "Seed/PeerManager.h"

#include <P2P/P2PServer.h>
#include <Database/Database.h>
#include <Config/Config.h>
#include <TxPool/TransactionPool.h>

class P2PServer : public IP2PServer
{
public:
	static std::shared_ptr<P2PServer> Create(
		const Config& config,
		std::shared_ptr<IBlockChainServer> pBlockChainServer,
		std::shared_ptr<IDatabase> pDatabase,
		std::shared_ptr<ITransactionPool> pTransactionPool
	);
	virtual ~P2PServer();

	virtual SyncStatusConstPtr GetSyncStatus() const override final { return m_pSyncStatus; }

	virtual std::pair<size_t, size_t> GetNumberOfConnectedPeers() const override final;
	virtual std::vector<Peer> GetAllPeers() const override final;
	virtual std::vector<ConnectedPeer> GetConnectedPeers() const override final;

	virtual std::optional<Peer> GetPeer(
		const IPAddress& address,
		const std::optional<uint16_t>& portOpt
	) const override final;

	virtual bool BanPeer(
		const IPAddress& address,
		const std::optional<uint16_t>& portOpt,
		const EBanReason banReason
	) override final;

	virtual bool UnbanPeer(
		const IPAddress& address,
		const std::optional<uint16_t>& portOpt
	) override final;

	virtual bool UnbanAllPeers() override final;

private:
	P2PServer(
		SyncStatusConstPtr pSyncStatus,
		Locked<PeerManager> peerManager,
		ConnectionManagerPtr pConnectionManager,
		std::shared_ptr<Pipeline> pPipeline,
		std::shared_ptr<Seeder> pSeeder,
		std::shared_ptr<Syncer> pSyncer,
		std::shared_ptr<Dandelion> pDandelion
	);

	SyncStatusConstPtr m_pSyncStatus;
	Locked<PeerManager> m_peerManager;
	ConnectionManagerPtr m_pConnectionManager;
	std::shared_ptr<Pipeline> m_pPipeline;
	std::shared_ptr<Seeder> m_pSeeder;
	std::shared_ptr<Syncer> m_pSyncer;
	std::shared_ptr<Dandelion> m_pDandelion;
};