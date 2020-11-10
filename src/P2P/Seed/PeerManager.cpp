#include "PeerManager.h"

#include <Core/Context.h>
#include <P2P/Common.h>
#include <Common/Util/TimeUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Config/Config.h>
#include <Common/Logger.h>
#include <Common/ThreadManager.h>
#include <Crypto/CSPRNG.h>

PeerManager::PeerManager(const Context::Ptr& pContext, std::shared_ptr<Locked<IPeerDB>> pPeerDB)
	: m_taskId(0), m_pContext(pContext), m_pPeerDB(pPeerDB)
{

}

PeerManager::~PeerManager()
{
	LOG_INFO("Shutting down peer manager");

	m_pContext->GetScheduler()->RemoveTask(m_taskId);
	Thread_ManagePeers(*this);
}

std::shared_ptr<Locked<PeerManager>> PeerManager::Create(const Context::Ptr& pContext, std::shared_ptr<Locked<IPeerDB>> pPeerDB)
{
	std::shared_ptr<PeerManager> pPeerManager(new PeerManager(pContext, pPeerDB));

	pPeerManager->m_peersByAddress.clear();
	const std::vector<PeerPtr> peers = pPeerDB->Read()->LoadAllPeers();
	for (const PeerPtr& peer : peers)
	{
		pPeerManager->m_peersByAddress.emplace(peer->GetIPAddress(), PeerEntry(peer));
	}

	std::shared_ptr<Locked<PeerManager>> pLocked = std::make_shared<Locked<PeerManager>>(Locked<PeerManager>(pPeerManager));

	std::weak_ptr<Locked<PeerManager>> pLockedWeak(pLocked);
	const uint64_t taskId = pContext->GetScheduler()->interval(std::chrono::seconds(15), [pLockedWeak]() {
		auto pLocked = pLockedWeak.lock();
		if (pLocked != nullptr)
		{
			auto pWriter = pLocked->Write();
			PeerManager::Thread_ManagePeers(*pWriter.GetShared());
		}
	});

	pPeerManager->SetTaskId(taskId);

	return pLocked;
}

void PeerManager::Thread_ManagePeers(PeerManager& peerManager)
{
	ThreadManagerAPI::SetCurrentThreadName("PEER_MANAGER");
	LOG_TRACE("BEGIN");

	try
	{
		std::vector<PeerPtr> peersToUpdate;
		std::vector<PeerPtr> peersToDelete;

		{
			const time_t minimumContactTime = std::chrono::system_clock::to_time_t(
				std::chrono::system_clock::now() - std::chrono::hours(24 * 7)
			);

			for (auto iter : peerManager.m_peersByAddress)
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
				peerManager.m_peersByAddress.erase(pPeer->GetIPAddress());
			}
		}

		peerManager.m_pPeerDB->Write()->SavePeers(peersToUpdate);
		peerManager.m_pPeerDB->Write()->DeletePeers(peersToDelete);
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	LOG_TRACE("END");
}

bool PeerManager::ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const
{
	const time_t currentTime = TimeUtil::Now();
	const time_t maxBanTime = std::chrono::system_clock::to_time_t(
		std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW)
	);

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

PeerPtr PeerManager::GetPeer(const IPAddress& address)
{
	auto iter = m_peersByAddress.find(address);
	if (iter != m_peersByAddress.cend())
	{
		return iter->second.m_peer;
	}

	PeerPtr pPeer = std::make_shared<Peer>(address);
	m_peersByAddress.emplace(address, PeerEntry(pPeer));
	return pPeer;
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

std::vector<PeerPtr> PeerManager::GetPeers(
	const Capabilities::ECapability& preferredCapability,
	const uint16_t maxPeers) const
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
			m_peersByAddress.emplace(ipAddress, PeerEntry(std::make_shared<Peer>(ipAddress)));
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
		PeerPtr peer = std::make_shared<Peer>(address, 0, Capabilities(0), "");
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

std::vector<PeerPtr> PeerManager::GetPeersWithCapability(
	const Capabilities::ECapability& preferredCapability,
	const uint16_t maxPeers,
	const bool connectingToPeer) const
{
	const size_t numPeers = m_peersByAddress.size();
	if (numPeers == 0)
	{
		return {};
	}

	std::vector<PeerPtr> peersFound;
	const time_t currentTime = TimeUtil::Now();
	const time_t maxBanTime = std::chrono::system_clock::to_time_t(
		std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW)
	);
	
	auto iter = m_peersByAddress.begin();
	std::advance(iter, CSPRNG::GenerateRandom(0, numPeers));

	for (size_t i = 0; i < numPeers; i++)
	{
		if (iter == m_peersByAddress.end())
		{
			iter = m_peersByAddress.begin();
		}

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

		iter++;
	}

	return peersFound;
}