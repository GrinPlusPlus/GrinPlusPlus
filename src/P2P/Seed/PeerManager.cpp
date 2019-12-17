#include "PeerManager.h"

#include <P2P/Common.h>
#include <Common/Util/TimeUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Config/Config.h>
#include <Infrastructure/Logger.h>
#include <Infrastructure/ThreadManager.h>

PeerManager::PeerManager(const Config& config, std::shared_ptr<Locked<IPeerDB>> pPeerDB)
	: m_config(config), m_pPeerDB(pPeerDB), m_terminate(false)
{

}

PeerManager::~PeerManager()
{
	m_terminate = true;
	ThreadUtil::Join(m_peerThread);
}

Locked<PeerManager> PeerManager::Create(const Config& config, std::shared_ptr<Locked<IPeerDB>> pPeerDB)
{
	std::shared_ptr<PeerManager> pPeerManager = std::shared_ptr<PeerManager>(new PeerManager(config, pPeerDB));

	pPeerManager->m_peersByAddress.clear();
	const std::vector<PeerPtr> peers = pPeerDB->Read()->LoadAllPeers();
	for (const PeerPtr& peer : peers)
	{
		pPeerManager->m_peersByAddress.emplace(peer->GetIPAddress(), PeerEntry(peer));
	}

	Locked<PeerManager> peerManager(pPeerManager);
	pPeerManager->m_peerThread = std::thread(PeerManager::Thread_ManagePeers, peerManager, std::ref(pPeerManager->m_terminate));

	return peerManager;
}

void PeerManager::Thread_ManagePeers(Locked<PeerManager> peerManager, const std::atomic_bool& terminate)
{
	ThreadManagerAPI::SetCurrentThreadName("PEER_MANAGER");
	LOG_TRACE("BEGIN");

	while (!terminate)
	{
		ThreadUtil::SleepFor(std::chrono::seconds(15), terminate);

		std::vector<PeerPtr> peersToUpdate;
		std::vector<PeerPtr> peersToDelete;

		{
			const time_t minimumContactTime = std::chrono::system_clock::to_time_t(
				std::chrono::system_clock::now() - std::chrono::hours(24 * 7)
			);

			auto pWriter = peerManager.Write();
			for (auto iter : pWriter->m_peersByAddress)
			{
				PeerEntry& peerEntry = iter.second;
				if (peerEntry.m_peer->IsDirty())
				{
					peersToUpdate.push_back(peerEntry.m_peer);
					peerEntry.m_peer->SetDirty(false);
				}
				else if (peerEntry.m_peer->GetLastContactTime() < minimumContactTime)
				{
					peersToDelete.push_back(peerEntry.m_peer);
				}
			}

			for (PeerPtr pPeer : peersToDelete)
			{
				pWriter->m_peersByAddress.erase(pPeer->GetIPAddress());
			}
		}

		peerManager.Write()->m_pPeerDB->Write()->SavePeers(peersToUpdate);
		peerManager.Write()->m_pPeerDB->Write()->DeletePeers(peersToDelete);
	}

	LOG_TRACE("END");
}

bool PeerManager::ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const
{
	const time_t currentTime = TimeUtil::Now();
	const time_t maxBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW));

	uint64_t peersFound = 0;
	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const PeerPtr& peer = peerEntry.m_peer;

		if (peer->GetLastBanTime() > maxBanTime)
		{
			continue;
		}

		const bool hasCapability = peer->GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (!peerEntry.m_peer->IsConnected() && std::difftime(currentTime, peerEntry.m_lastAttempt) > P2P::RETRY_WINDOW)
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

std::optional<PeerPtr> PeerManager::GetPeer(const IPAddress& address)
{
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.cend())
	{
		return std::make_optional(iter->second.m_peer);
	}

	return m_pPeerDB->Read()->GetPeer(address, std::nullopt);
}

std::optional<PeerConstPtr> PeerManager::GetPeer(const IPAddress& address) const
{
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.cend())
	{
		return std::make_optional(iter->second.m_peer);
	}

	return m_pPeerDB->Read()->GetPeer(address, std::nullopt);
}

PeerPtr PeerManager::GetNewPeer(const Capabilities::ECapability& preferredCapability)
{
	std::vector<PeerPtr> peers = GetPeersWithCapability(preferredCapability, 1, true);
	if (peers.empty())
	{
		peers = GetPeersWithCapability(Capabilities::UNKNOWN, 1, true);
	}
	
	if (peers.empty())
	{
		return nullptr;
	}

	return peers.front();
}

std::vector<PeerPtr> PeerManager::GetAllPeers()
{
	std::vector<PeerPtr> peers;
	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerPtr peer = iter->second.m_peer;
		if (peer->GetLastContactTime() > 0)
		{
			peers.push_back(peer);
		}
	}

	return peers;
}

std::vector<PeerConstPtr> PeerManager::GetAllPeers() const
{
	std::vector<PeerConstPtr> peers;
	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerConstPtr peer = iter->second.m_peer;
		if (peer->GetLastContactTime() > 0)
		{
			peers.push_back(peer);
		}
	}

	return peers;
}

std::vector<PeerPtr> PeerManager::GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const
{
	std::vector<PeerPtr> peers = GetPeersWithCapability(preferredCapability, maxPeers, false);
	if (peers.empty())
	{
		peers = GetPeersWithCapability(Capabilities::UNKNOWN, maxPeers, false);
	}

	return peers;
}

void PeerManager::AddFreshPeers(const std::vector<SocketAddress>& peerAddresses)
{
	for (auto socketAddress : peerAddresses)
	{
		const IPAddress& ipAddress = socketAddress.GetIPAddress();
		if (m_peersByAddress.find(ipAddress) == m_peersByAddress.end())
		{
			m_peersByAddress.emplace(ipAddress, PeerEntry(std::make_shared<Peer>(socketAddress)));
		}
	}
}

void PeerManager::BanPeer(const IPAddress& address, const EBanReason banReason)
{
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.end())
	{
		iter->second.m_peer->Ban(banReason);
	}
	else
	{
		PeerPtr peer = std::make_shared<Peer>(SocketAddress(address, 0), 0, Capabilities(0), "");
		peer->Ban(banReason);
		m_peersByAddress.emplace(address, PeerEntry(peer, TimeUtil::Now()));
	}
}

void PeerManager::UnbanPeer(const IPAddress& address)
{
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.end())
	{
		iter->second.m_peer->Unban();
	}
}

std::vector<PeerPtr> PeerManager::GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers, const bool connectingToPeer) const
{
	std::vector<PeerPtr> peersFound;
	const time_t currentTime = TimeUtil::Now();
	const time_t maxBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW));

	for (auto iter = m_peersByAddress.begin(); iter != m_peersByAddress.end(); iter++)
	{
		PeerEntry& peerEntry = iter->second;
		const PeerPtr& peer = peerEntry.m_peer;

		if (connectingToPeer && peer->GetLastBanTime() > maxBanTime)
		{
			continue;
		}

		const bool hasCapability = peer->GetCapabilities().HasCapability(preferredCapability);
		if (hasCapability)
		{
			if (!connectingToPeer || (!peerEntry.m_peer->IsConnected() && std::difftime(currentTime, peerEntry.m_lastAttempt) > P2P::RETRY_WINDOW))
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