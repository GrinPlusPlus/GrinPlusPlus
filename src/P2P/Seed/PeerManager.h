#pragma once

#include <Core/Context.h>
#include <Net/IPAddress.h>
#include <Net/SocketAddress.h>
#include <P2P/Peer.h>
#include <P2P/Capabilities.h>
#include <P2P/BanReason.h>
#include <Database/PeerDB.h>
#include <Config/Config.h>
#include <Core/Traits/Lockable.h>

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <thread>

class PeerManager
{
public:
	static Locked<PeerManager> Create(const Context::Ptr& pContext, std::shared_ptr<Locked<IPeerDB>> pPeerDB);
	~PeerManager();

	bool ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const;

	PeerPtr GetPeer(const IPAddress& address);
	std::optional<PeerConstPtr> GetPeer(const IPAddress& address) const;

	std::vector<PeerPtr> GetAllPeers();
	std::vector<PeerConstPtr> GetAllPeers() const;

	PeerPtr GetNewPeer(const Capabilities::ECapability& preferredCapability);
	std::vector<PeerPtr> GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const;

	void AddFreshPeers(const std::vector<SocketAddress>& peerAddresses);
	//void SetPeerConnected(const PeerPtr& peer, const bool connected);
	//void BanPeer(PeerPtr peer, const EBanReason banReason);
	void BanPeer(const IPAddress& address, const EBanReason banReason);
	void UnbanPeer(const IPAddress& address);
	// TODO: RemovePeer

private:
	PeerManager(const Context::Ptr& pContext, std::shared_ptr<Locked<IPeerDB>> pPeerDB);

	struct PeerEntry
	{
		PeerEntry(PeerPtr pPeer)
			: m_peer(pPeer), m_lastAttempt(0)//, m_connected(false), m_dirty(false)
		{

		}

		PeerEntry(PeerPtr pPeer, const time_t& lastAttempt)
			: m_peer(pPeer), m_lastAttempt(lastAttempt)
		{

		}

		PeerPtr m_peer;
		time_t m_lastAttempt;
	};

	static void Thread_ManagePeers(PeerManager& peerManager);

	std::vector<PeerPtr> GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers, const bool connectingToPeer) const;

	Context::Ptr m_pContext;
	std::shared_ptr<Locked<IPeerDB>> m_pPeerDB;

	std::atomic_bool m_terminate;
	std::thread m_peerThread;

	mutable std::unordered_set<IPAddress> m_peersServed;
	mutable std::map<IPAddress, PeerEntry> m_peersByAddress;
};