#include "P2PServerImpl.h"

#include "Seed/Seeder.h"
#include "Pipeline/Pipeline.h"
#include "Sync/Syncer.h"
#include <BlockChain/BlockChainServer.h>

P2PServer::P2PServer(
	SyncStatusConstPtr pSyncStatus,
	Locked<PeerManager> peerManager,
	ConnectionManagerPtr pConnectionManager,
	std::shared_ptr<Pipeline> pPipeline,
	std::shared_ptr<Seeder> pSeeder,
	std::shared_ptr<Syncer> pSyncer,
	std::shared_ptr<Dandelion> pDandelion)
	: m_pSyncStatus(pSyncStatus),
	m_peerManager(peerManager),
	m_pConnectionManager(pConnectionManager),
	m_pPipeline(pPipeline),
	m_pSeeder(pSeeder),
	m_pSyncer(pSyncer),
	m_pDandelion(pDandelion)
{

}

P2PServer::~P2PServer()
{
	m_pDandelion.reset();
	m_pSyncer.reset();
	m_pSeeder.reset();
	m_pPipeline.reset();
	m_pConnectionManager->Shutdown();
}

std::shared_ptr<P2PServer> P2PServer::Create(
	const Config& config,
	std::shared_ptr<IBlockChainServer> pBlockChainServer,
	TxHashSetManagerConstPtr pTxHashSetManager,
	std::shared_ptr<IDatabase> pDatabase,
	std::shared_ptr<ITransactionPool> pTransactionPool)
{
	// Sync Status
	SyncStatusPtr pSyncStatus(new SyncStatus());

	// Peer Manager
	Locked<PeerManager> peerManager = PeerManager::Create(
		config,
		pDatabase->GetPeerDB()
	);

	// Connection Manager
	ConnectionManagerPtr pConnectionManager = ConnectionManager::Create();

	// Pipeline
	std::shared_ptr<Pipeline> pPipeline = Pipeline::Create(
		config,
		pConnectionManager,
		pBlockChainServer,
		pSyncStatus
	);

	// Seeder
	std::shared_ptr<Seeder> pSeeder = Seeder::Create(
		config,
		*pConnectionManager,
		peerManager,
		pBlockChainServer,
		pPipeline,
		pSyncStatus
	);

	// Syncer
	std::shared_ptr<Syncer> pSyncer = Syncer::Create(
		pConnectionManager,
		pBlockChainServer,
		pPipeline,
		pSyncStatus
	);

	// Dandelion
	std::shared_ptr<Dandelion> pDandelion = Dandelion::Create(
		config,
		*pConnectionManager,
		pBlockChainServer,
		pTxHashSetManager,
		pTransactionPool,
		pDatabase->GetBlockDB()
	);

	// P2P Server
	return std::shared_ptr<P2PServer>(new P2PServer(
		pSyncStatus,
		peerManager,
		pConnectionManager,
		pPipeline,
		pSeeder,
		pSyncer,
		pDandelion
	));
}

std::pair<size_t, size_t> P2PServer::GetNumberOfConnectedPeers() const
{
	return std::make_pair(
		m_pConnectionManager->GetNumInbound(),
		m_pConnectionManager->GetNumOutbound()
	);
}

std::vector<Peer> P2PServer::GetAllPeers() const
{
	return m_peerManager.Read()->GetAllPeers();
}

std::vector<ConnectedPeer> P2PServer::GetConnectedPeers() const
{
	return m_pConnectionManager->GetConnectedPeers();
}

std::optional<Peer> P2PServer::GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	std::optional<std::pair<uint64_t, ConnectedPeer>> connectedPeerOpt = m_pConnectionManager->GetConnectedPeer(address, portOpt);
	if (connectedPeerOpt.has_value())
	{
		return std::make_optional(connectedPeerOpt.value().second.GetPeer());
	}

	return m_peerManager.Read()->GetPeer(address, portOpt);
}

bool P2PServer::BanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt, const EBanReason banReason)
{
	std::optional<std::pair<uint64_t, ConnectedPeer>> connectedPeerOpt = m_pConnectionManager->GetConnectedPeer(address, portOpt);
	if (connectedPeerOpt.has_value())
	{
		m_pConnectionManager->BanConnection(connectedPeerOpt.value().first, EBanReason::ManualBan);
		return true;
	}

	std::optional<Peer> peerOpt = m_peerManager.Read()->GetPeer(address, portOpt);
	if (peerOpt.has_value())
	{
		m_peerManager.Write()->BanPeer(peerOpt.value(), EBanReason::ManualBan);
		return true;
	}

	return false;
}

bool P2PServer::UnbanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt)
{
	std::optional<Peer> peerOpt = m_peerManager.Read()->GetPeer(address, portOpt);
	if (peerOpt.has_value())
	{
		Peer peer = peerOpt.value();
		if (peer.IsBanned())
		{
			m_peerManager.Write()->UnbanPeer(address);
		}

		return true;
	}

	return false;
}

bool P2PServer::UnbanAllPeers()
{
	std::vector<Peer> peers = m_peerManager.Read()->GetAllPeers();
	for (const Peer& peer : peers)
	{
		if (peer.IsBanned())
		{
			const IPAddress& address = peer.GetIPAddress();
			m_peerManager.Write()->UnbanPeer(address);
		}
	}

	return true;
}

namespace P2PAPI
{
	EXPORT std::shared_ptr<IP2PServer> StartP2PServer(
		const Config& config,
		IBlockChainServerPtr pBlockChainServer,
		TxHashSetManagerConstPtr pTxHashSetManager,
		IDatabasePtr pDatabase,
		ITransactionPoolPtr pTransactionPool)
	{
		return P2PServer::Create(config, pBlockChainServer, pTxHashSetManager, pDatabase, pTransactionPool);
	}
}