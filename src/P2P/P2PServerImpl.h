#pragma once

#include "Dandelion.h"
#include "ConnectionManager.h"
#include "Pipeline/Pipeline.h"
#include "Sync/Syncer.h"
#include "Seed/Seeder.h"
#include "Seed/PeerManager.h"

#include <P2P/P2PServer.h>
#include <Database/Database.h>
#include <Core/Config.h>
#include <TxPool/TransactionPool.h>

class P2PServer : public IP2PServer
{
public:
	static std::shared_ptr<P2PServer> Create(
		const std::shared_ptr<Context>& pContext,
		const std::shared_ptr<IBlockChain>& pBlockChain,
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		std::shared_ptr<IDatabase> pDatabase,
		const ITransactionPool::Ptr& pTransactionPool
	);
	virtual ~P2PServer();

	SyncStatusConstPtr GetSyncStatus() const final { return m_pSyncStatus; }

	std::pair<size_t, size_t> GetNumberOfConnectedPeers() const final;
	std::vector<PeerConstPtr> GetAllPeers() const final;
	std::vector<ConnectedPeer> GetConnectedPeers() const final;
	std::optional<PeerConstPtr> GetPeer(const IPAddress& address) const final;

	void BanPeer(const IPAddress& address, const EBanReason banReason) final;
	void UnbanPeer(const IPAddress& address) final;
	bool UnbanAllPeers() final;

	void BroadcastTransaction(const TransactionPtr& pTransaction) final;

private:
	P2PServer(
		SyncStatusConstPtr pSyncStatus,
		const std::shared_ptr<Locked<PeerManager>>& pPeerManager,
		ConnectionManagerPtr pConnectionManager,
		std::shared_ptr<Pipeline> pPipeline,
		std::unique_ptr<Seeder>&& pSeeder,
		std::shared_ptr<Syncer> pSyncer,
		std::shared_ptr<Dandelion> pDandelion
	);

	SyncStatusConstPtr m_pSyncStatus;
	std::shared_ptr<Locked<PeerManager>> m_pPeerManager;
	ConnectionManagerPtr m_pConnectionManager;
	std::shared_ptr<Pipeline> m_pPipeline;
	std::unique_ptr<Seeder> m_pSeeder;
	std::shared_ptr<Syncer> m_pSyncer;
	std::shared_ptr<Dandelion> m_pDandelion;
};