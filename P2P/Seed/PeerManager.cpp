#include "PeerManager.h"
#include "../Common.h"

#include <Config/Config.h>

PeerManager::PeerManager(const Config& config, IPeerDB& peerDB)
	: m_config(config), m_peerDB(peerDB)
{

}

void PeerManager::Initialize()
{
	std::unique_lock<std::shared_mutex> readLock(m_peersMutex);

	const std::vector<Peer> peers = m_peerDB.LoadAllPeers();
	for (const Peer& peer : peers)
	{
		m_peersByAddress.emplace(peer.GetIPAddress(), peer);
	}
}

bool PeerManager::ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const
{
	// TODO: Implement
	return true;
}

std::unique_ptr<Peer> PeerManager::GetNewPeer(const Capabilities::ECapability& preferredCapability) const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	std::vector<Peer> peers = GetPeersWithCapability(preferredCapability, 1);
	if (peers.empty())
	{
		peers = GetPeersWithCapability(Capabilities::UNKNOWN, 1);
	}
	
	if (peers.empty())
	{
		return std::unique_ptr<Peer>(nullptr);
	}

	return std::make_unique<Peer>(peers.front());
}

std::vector<Peer> PeerManager::GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	std::vector<Peer> peers = GetPeersWithCapability(preferredCapability, maxPeers);
	if (peers.empty())
	{
		peers = GetPeersWithCapability(Capabilities::UNKNOWN, maxPeers);
	}

	return peers;
}

void PeerManager::AddPeerAddresses(const std::vector<SocketAddress>& peerAddresses)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	for (auto peerAddress : peerAddresses)
	{
		const Peer peer(peerAddress);
		AddPeer(peer);
	}
}

bool PeerManager::UpdatePeer(const Peer& peer)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	const IPAddress& address = peer.GetIPAddress();
	if (m_peersByAddress.find(address) != m_peersByAddress.cend())
	{
		m_peersByAddress.emplace(address, peer);
		m_peerDB.AddPeers(std::vector<Peer>({ peer }));

		return true;
	}
	else
	{
		AddPeer(peer);
		return false;
	}
}

bool PeerManager::AddPeer(const Peer& peer)
{
	const IPAddress& address = peer.GetIPAddress();
	if (m_peersByAddress.find(address) == m_peersByAddress.cend())
	{
		m_peersByAddress.emplace(address, peer);
		m_peerDB.AddPeers(std::vector<Peer>({ peer }));

		return true;
	}

	return false;
}

std::vector<Peer> PeerManager::GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const
{
	std::vector<Peer> peersFound;

	for (auto iter = m_peersByAddress.cbegin(); iter != m_peersByAddress.cend(); iter++)
	{
		const Peer& peer = iter->second;

		const bool hasCapability = peer.GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (m_peersServed.find(peer.GetIPAddress()) == m_peersServed.cend())
			{
				m_peersServed.insert(peer.GetIPAddress());

				peersFound.push_back(peer);
				if (peersFound.size() == maxPeers)
				{
					return peersFound;
				}
			}
		}
	}

	return peersFound;
}