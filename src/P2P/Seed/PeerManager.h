#pragma once

#include <Net/IPAddress.h>
#include <Net/SocketAddress.h>
#include <P2P/Peer.h>
#include <P2P/Capabilities.h>
#include <P2P/BanReason.h>
#include <Database/PeerDB.h>
#include <Core/Config.h>
#include <Core/Traits/Lockable.h>

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <thread>

// Forward Declarations
class Context;

class PeerManager
{
public:
	static std::shared_ptr<Locked<PeerManager>> Create(const std::shared_ptr<Context>& pContext, std::shared_ptr<Locked<IPeerDB>> pPeerDB);
	~PeerManager();

	PeerPtr GetPeer(const IPAddress& address);
	std::optional<PeerConstPtr> GetPeer(const IPAddress& address) const;

	std::vector<PeerPtr> GetAllPeers();
	std::vector<PeerConstPtr> GetAllPeers() const;

	PeerPtr GetNewPeer(const Capabilities::ECapability& preferredCapability);
	std::vector<PeerPtr> GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const;

	void AddFreshPeers(const std::vector<SocketAddress>& peerAddresses);
	void BanPeer(const IPAddress& address, const EBanReason banReason);
	void UnbanPeer(const IPAddress& address);

private:
	PeerManager(const std::shared_ptr<Context>& pContext, std::shared_ptr<Locked<IPeerDB>> pPeerDB);

	struct PeerEntry
	{
		PeerEntry(const PeerPtr& pPeer)
			: m_peer(pPeer), m_lastAttempt(0)//, m_connected(false), m_dirty(false)
		{

		}

		PeerEntry(const PeerPtr& pPeer, const time_t& lastAttempt)
			: m_peer(pPeer), m_lastAttempt(lastAttempt)
		{

		}

		PeerPtr m_peer;
		time_t m_lastAttempt;
	};

	static void Thread_ManagePeers(PeerManager& peerManager);

	std::vector<PeerPtr> GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers, const bool connectingToPeer) const;

	void SetTaskId(const uint64_t taskId) noexcept { m_taskId = taskId; }
	uint64_t m_taskId;

	std::shared_ptr<Context> m_pContext;
	std::shared_ptr<Locked<IPeerDB>> m_pPeerDB;

	mutable std::map<IPAddress, PeerEntry> m_peersByAddress;
};