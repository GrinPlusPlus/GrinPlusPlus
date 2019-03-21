#include "PeerManager.h"

#include <P2P/Common.h>
#include <Config/Config.h>

PeerManager::PeerManager(const Config& config, IPeerDB& peerDB)
	: m_config(config), m_peerDB(peerDB)
{

}

void PeerManager::Initialize()
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	const std::vector<Peer> peers = m_peerDB.LoadAllPeers();
	for (const Peer& peer : peers)
	{
		m_peersByAddress.emplace(peer.GetIPAddress(), peer);
	}
}

bool PeerManager::ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	uint64_t peersFound = 0;
	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const Peer& peer = peerEntry.m_peer;

		if (peer.IsBanned())
		{
			continue;
		}

		const bool hasCapability = peer.GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (!peerEntry.m_peerServed)
			{
				++peersFound;
			}
		}
	}

	return peersFound < 100;
}

std::optional<Peer> PeerManager::GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.cend())
	{
		return std::make_optional<Peer>(iter->second.m_peer);
	}

	return m_peerDB.GetPeer(address, portOpt);
}

std::unique_ptr<Peer> PeerManager::GetNewPeer(const Capabilities::ECapability& preferredCapability) const
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	std::vector<Peer> peers = GetPeersWithCapability(preferredCapability, 1, true);
	if (peers.empty())
	{
		peers = GetPeersWithCapability(Capabilities::UNKNOWN, 1, true);
	}
	
	if (peers.empty())
	{
		return std::unique_ptr<Peer>(nullptr);
	}

	return std::make_unique<Peer>(peers.front());
}

std::vector<Peer> PeerManager::GetAllPeers() const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	std::vector<Peer> peers;
	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const Peer& peer = peerEntry.m_peer;

		if (peer.GetLastContactTime() > 0)
		{
			peers.push_back(peer);
		}
	}

	return peers;
}

std::vector<Peer> PeerManager::GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	std::vector<Peer> peers = GetPeersWithCapability(preferredCapability, maxPeers, false);
	if (peers.empty())
	{
		peers = GetPeersWithCapability(Capabilities::UNKNOWN, maxPeers, false);
	}

	return peers;
}

void PeerManager::AddPeerAddresses(const std::vector<SocketAddress>& peerAddresses)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	for (auto peerAddress : peerAddresses)
	{
		const Peer peer(peerAddress);
		AddPeer(peer, false);
	}
}

bool PeerManager::UpdatePeer(const Peer& peer)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	m_peerDB.AddPeers(std::vector<Peer>({ peer }));

	const IPAddress& address = peer.GetIPAddress();
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.end())
	{
		iter->second.m_peer = peer;
		iter->second.m_peerServed = true;
		return true;
	}
	else
	{
		AddPeer(peer, true);
		return false;
	}
}

bool PeerManager::BanPeer(Peer& peer, const EBanReason banReason)
{
	peer.UpdateLastBanTime();
	peer.UpdateBanReason(banReason);

	return UpdatePeer(peer);
}

bool PeerManager::AddPeer(const Peer& peer, const bool served)
{
	const IPAddress& address = peer.GetIPAddress();
	if (m_peersByAddress.find(address) == m_peersByAddress.cend())
	{
		m_peersByAddress.emplace(address, PeerEntry(peer));
		return true;
	}

	return false;
}

std::vector<Peer> PeerManager::GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers, const bool connectingToPeer) const
{
	std::vector<Peer> peersFound;

	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const Peer& peer = peerEntry.m_peer;

		if (connectingToPeer && peer.IsBanned())
		{
			continue;
		}

		const bool hasCapability = peer.GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (!connectingToPeer || !peerEntry.m_peerServed)
			{
				peerEntry.m_peerServed = (peerEntry.m_peerServed || connectingToPeer);

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