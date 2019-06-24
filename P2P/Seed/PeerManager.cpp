#include "PeerManager.h"

#include <P2P/Common.h>
#include <Common/Util/TimeUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Config/Config.h>
#include <Infrastructure/Logger.h>
#include <Infrastructure/ThreadManager.h>

PeerManager::PeerManager(const Config& config, IPeerDB& peerDB)
	: m_config(config), m_peerDB(peerDB)
{

}

void PeerManager::Start()
{
	m_terminate = true;

	if (m_peerThread.joinable())
	{
		m_peerThread.join();
	}

	m_terminate = false;

	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	m_peersByAddress.clear();
	const std::vector<Peer> peers = m_peerDB.LoadAllPeers();
	for (const Peer& peer : peers)
	{
		m_peersByAddress.emplace(peer.GetIPAddress(), PeerEntry(Peer(peer)));
	}

	m_peerThread = std::thread(Thread_ManagePeers, std::ref(*this));
}

void PeerManager::Stop()
{
	m_terminate = true;

	if (m_peerThread.joinable())
	{
		m_peerThread.join();
	}
}

void PeerManager::Thread_ManagePeers(PeerManager& peerManager)
{
	ThreadManagerAPI::SetCurrentThreadName("PEER_MANAGER_THREAD");
	LoggerAPI::LogTrace("PeerManager::Thread_ManagePeers() - BEGIN");

	while (!peerManager.m_terminate)
	{
		ThreadUtil::SleepFor(std::chrono::seconds(15), peerManager.m_terminate);

		std::vector<Peer> peersToUpdate;

		std::shared_lock<std::shared_mutex> readLock(peerManager.m_peersMutex);
		for (auto iter = peerManager.m_peersByAddress.begin(); iter != peerManager.m_peersByAddress.end(); iter++)
		{
			PeerEntry& peerEntry = iter->second;
			if (peerEntry.m_dirty)
			{
				peersToUpdate.push_back(peerEntry.m_peer);
				peerEntry.m_dirty = false;
			}
		}

		readLock.unlock();

		peerManager.m_peerDB.SavePeers(peersToUpdate);
	}

	LoggerAPI::LogTrace("PeerManager::Thread_ManagePeers() - END");
}

bool PeerManager::ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const
{
	std::shared_lock<std::shared_mutex> readLock(m_peersMutex);

	const time_t currentTime = TimeUtil::Now();
	const time_t maxBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW));

	uint64_t peersFound = 0;
	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const Peer& peer = peerEntry.m_peer;

		if (peer.GetLastBanTime() > maxBanTime)
		{
			continue;
		}

		const bool hasCapability = peer.GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (!peerEntry.m_connected && std::difftime(currentTime, peerEntry.m_lastAttempt) > P2P::RETRY_WINDOW)
			{
				++peersFound;
				if (peersFound >= 100)
				{
					return false;
				}
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

void PeerManager::AddFreshPeers(const std::vector<SocketAddress>& peerAddresses)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	for (auto socketAddress : peerAddresses)
	{
		const IPAddress& ipAddress = socketAddress.GetIPAddress();
		if (m_peersByAddress.find(ipAddress) == m_peersByAddress.end())
		{
			m_peersByAddress.emplace(ipAddress, PeerEntry(Peer(socketAddress)));
		}
	}
}

void PeerManager::SetPeerConnected(const Peer& peer, const bool connected)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	const IPAddress& address = peer.GetIPAddress();
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.end())
	{
		iter->second.m_connected = connected;
		iter->second.m_peer = peer;
		iter->second.m_dirty = true;
	}
	else
	{
		m_peersByAddress.emplace(address, PeerEntry(peer, TimeUtil::Now(), connected, true));
	}
}

void PeerManager::BanPeer(Peer& peer, const EBanReason banReason)
{
	peer.UpdateLastBanTime();
	peer.UpdateBanReason(banReason);

	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	const IPAddress& address = peer.GetIPAddress();
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.end())
	{
		iter->second.m_connected = false;
		iter->second.m_peer = peer;
		iter->second.m_dirty = true;
	}
	else
	{
		m_peersByAddress.emplace(address, PeerEntry(peer, TimeUtil::Now(), false, true));
	}
}

void PeerManager::UnbanPeer(const IPAddress& address)
{
	std::unique_lock<std::shared_mutex> writeLock(m_peersMutex);

	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.end())
	{
		iter->second.m_peer.Unban();
		iter->second.m_dirty = true;
	}
}

std::vector<Peer> PeerManager::GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers, const bool connectingToPeer) const
{
	std::vector<Peer> peersFound;
	const time_t currentTime = TimeUtil::Now();
	const time_t maxBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW));

	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const Peer& peer = peerEntry.m_peer;

		if (connectingToPeer && peer.GetLastBanTime() > maxBanTime)
		{
			continue;
		}

		const bool hasCapability = peer.GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (!connectingToPeer || (!peerEntry.m_connected && std::difftime(currentTime, peerEntry.m_lastAttempt) > P2P::RETRY_WINDOW))
			{
				if (connectingToPeer)
				{
					peerEntry.m_lastAttempt = currentTime;
				}

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