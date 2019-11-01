#include "P2PServerImpl.h"

#include <BlockChain/BlockChainServer.h>

P2PServer::P2PServer(const Config& config, IBlockChainServerPtr pBlockChainServer, IDatabasePtr pDatabase, ITransactionPoolPtr pTransactionPool)
	: m_config(config), 
	m_pBlockChainServer(pBlockChainServer), 
	m_pDatabase(pDatabase), 
	m_peerManager(config, pDatabase->GetPeerDB()),
	m_connectionManager(config, m_peerManager, pDatabase->GetBlockDB(), pBlockChainServer, pTransactionPool)
{

}

P2PServer::~P2PServer()
{
	m_connectionManager.Stop();
}

std::shared_ptr<P2PServer> P2PServer::Create(
	const Config& config,
	std::shared_ptr<IBlockChainServer> pBlockChainServer,
	std::shared_ptr<IDatabase> pDatabase,
	std::shared_ptr<ITransactionPool> pTransactionPool)
{
	P2PServer* pServer = new P2PServer(config, pBlockChainServer, pDatabase, pTransactionPool);
	pServer->m_connectionManager.Start();

	return std::shared_ptr<P2PServer>(pServer);
}

std::pair<size_t, size_t> P2PServer::GetNumberOfConnectedPeers() const
{
	return m_connectionManager.GetNumConnectionsWithDirection();
}

const SyncStatus& P2PServer::GetSyncStatus() const
{
	return m_connectionManager.GetSyncStatus();
}

std::vector<Peer> P2PServer::GetAllPeers() const
{
	return m_peerManager.GetAllPeers();
}

std::vector<ConnectedPeer> P2PServer::GetConnectedPeers() const
{
	return m_connectionManager.GetConnectedPeers();
}

std::optional<Peer> P2PServer::GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	std::optional<std::pair<uint64_t, ConnectedPeer>> connectedPeerOpt = m_connectionManager.GetConnectedPeer(address, portOpt);
	if (connectedPeerOpt.has_value())
	{
		return std::make_optional<Peer>(connectedPeerOpt.value().second.GetPeer());
	}

	return m_peerManager.GetPeer(address, portOpt);
}

bool P2PServer::BanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt, const EBanReason banReason)
{
	std::optional<std::pair<uint64_t, ConnectedPeer>> connectedPeerOpt = m_connectionManager.GetConnectedPeer(address, portOpt);
	if (connectedPeerOpt.has_value())
	{
		m_connectionManager.BanConnection(connectedPeerOpt.value().first, EBanReason::ManualBan);
		return true;
	}

	std::optional<Peer> peerOpt = m_peerManager.GetPeer(address, portOpt);
	if (peerOpt.has_value())
	{
		m_peerManager.BanPeer(peerOpt.value(), EBanReason::ManualBan);
		return true;
	}

	return false;
}

bool P2PServer::UnbanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt)
{
	std::optional<Peer> peerOpt = m_peerManager.GetPeer(address, portOpt);
	if (peerOpt.has_value())
	{
		Peer peer = peerOpt.value();
		if (peer.IsBanned())
		{
			const IPAddress& address = peer.GetIPAddress();
			m_peerManager.UnbanPeer(address);
		}

		return true;
	}

	return false;
}

bool P2PServer::UnbanAllPeers()
{
	std::vector<Peer> peers = m_peerManager.GetAllPeers();
	for (const Peer& peer : peers)
	{
		if (peer.IsBanned())
		{
			const IPAddress& address = peer.GetIPAddress();
			m_peerManager.UnbanPeer(address);
		}
	}

	return true;
}

namespace P2PAPI
{
	EXPORT std::shared_ptr<IP2PServer> StartP2PServer(const Config& config, IBlockChainServerPtr pBlockChainServer, IDatabasePtr pDatabase, ITransactionPoolPtr pTransactionPool)
	{
		return P2PServer::Create(config, pBlockChainServer, pDatabase, pTransactionPool);
	}
}