#include "P2PServerImpl.h"

#include "Seed/Seeder.h"
#include "Pipeline/Pipeline.h"
#include "Sync/Syncer.h"
#include "Messages/TransactionKernelMessage.h"

#include <Core/Context.h>
#include <BlockChain/BlockChain.h>

P2PServer::P2PServer(
	SyncStatusConstPtr pSyncStatus,
	const std::shared_ptr<Locked<PeerManager>>& pPeerManager,
	ConnectionManagerPtr pConnectionManager,
	std::shared_ptr<Pipeline> pPipeline,
	std::unique_ptr<Seeder>&& pSeeder,
	std::shared_ptr<Syncer> pSyncer,
	std::shared_ptr<Dandelion> pDandelion)
	: m_pSyncStatus(pSyncStatus),
	m_pPeerManager(pPeerManager),
	m_pConnectionManager(pConnectionManager),
	m_pPipeline(pPipeline),
	m_pSeeder(std::move(pSeeder)),
	m_pSyncer(pSyncer),
	m_pDandelion(pDandelion)
{

}

P2PServer::~P2PServer()
{
	LOG_INFO("Shutting down P2P server");
	m_pDandelion.reset();
	m_pSyncer.reset();
	m_pSeeder.reset();
	m_pPipeline.reset();
	m_pConnectionManager->Shutdown();
}

std::shared_ptr<P2PServer> P2PServer::Create(
	const Context::Ptr& pContext,
	const std::shared_ptr<IBlockChain>& pBlockChain,
	std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
	std::shared_ptr<IDatabase> pDatabase,
	const ITransactionPool::Ptr& pTransactionPool)
{
	// Sync Status
	SyncStatusPtr pSyncStatus(new SyncStatus());

	// Peer Manager
	std::shared_ptr<Locked<PeerManager>> peerManager = PeerManager::Create(
		pContext,
		pDatabase->GetPeerDB()
	);

	// Connection Manager
	ConnectionManagerPtr pConnectionManager = ConnectionManager::Create();

	const Config& config = pContext->GetConfig();

	// Pipeline
	std::shared_ptr<Pipeline> pPipeline = Pipeline::Create(
		config,
		pConnectionManager,
		pBlockChain,
		pSyncStatus
	);

	// Seeder
	std::unique_ptr<Seeder> pSeeder = Seeder::Create(
		pContext,
		*pConnectionManager,
		*peerManager,
		pBlockChain,
		pPipeline,
		pSyncStatus
	);

	// Syncer
	std::shared_ptr<Syncer> pSyncer = Syncer::Create(
		pConnectionManager,
		pBlockChain,
		pPipeline,
		pSyncStatus
	);

	// Dandelion
	std::shared_ptr<Dandelion> pDandelion = Dandelion::Create(
		config,
		*pConnectionManager,
		pBlockChain,
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
		std::move(pSeeder),
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

std::vector<PeerConstPtr> P2PServer::GetAllPeers() const
{
	return m_pPeerManager->Read()->GetAllPeers();
}

std::vector<ConnectedPeer> P2PServer::GetConnectedPeers() const
{
	return m_pConnectionManager->GetConnectedPeers();
}

std::optional<PeerConstPtr> P2PServer::GetPeer(const IPAddress& address) const
{
	std::optional<std::pair<uint64_t, ConnectedPeer>> connectedPeerOpt = m_pConnectionManager->GetConnectedPeer(address);
	if (connectedPeerOpt.has_value())
	{
		return std::make_optional(connectedPeerOpt.value().second.GetPeer());
	}

	return m_pPeerManager->Read()->GetPeer(address);
}

bool P2PServer::BanPeer(const IPAddress& address, const EBanReason banReason)
{
	std::optional<PeerPtr> peerOpt = m_pPeerManager->Write()->GetPeer(address);
	if (peerOpt.has_value())
	{
		peerOpt.value()->Ban(banReason);
		return true;
	}

	return false;
}

void P2PServer::UnbanPeer(const IPAddress& address)
{
	m_pPeerManager->Write()->UnbanPeer(address);
}

bool P2PServer::UnbanAllPeers()
{
	std::vector<PeerPtr> peers = m_pPeerManager->Write()->GetAllPeers();
	for (PeerPtr peer : peers)
	{
		if (peer->IsBanned())
		{
			peer->Unban();
		}
	}

	return true;
}

void P2PServer::BroadcastTransaction(const TransactionPtr& pTransaction)
{
	for (auto& kernel : pTransaction->GetKernels())
	{
		const TransactionKernelMessage message(kernel.GetHash());
		m_pConnectionManager->BroadcastMessage(message, 0);
	}
}

namespace P2PAPI
{
	std::shared_ptr<IP2PServer> StartP2PServer(
		const Context::Ptr& pContext,
		const IBlockChain::Ptr& pBlockChain,
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		IDatabasePtr pDatabase,
		const ITransactionPool::Ptr& pTransactionPool)
	{
		return P2PServer::Create(pContext, pBlockChain, pTxHashSetManager, pDatabase, pTransactionPool);
	}
}