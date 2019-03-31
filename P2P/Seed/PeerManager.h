#pragma once

#include <Net/IPAddress.h>
#include <Net/SocketAddress.h>
#include <P2P/peer.h>
#include <P2P/capabilities.h>
#include <P2P/BanReason.h>
#include <Database/PeerDB.h>
#include <Config/Config.h>

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <shared_mutex>

class PeerManager
{
public:
	PeerManager(const Config& config, IPeerDB& peerDB);
	
	void Start();
	void Stop();

	bool ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const;

	std::optional<Peer> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const;
	std::unique_ptr<Peer> GetNewPeer(const Capabilities::ECapability& preferredCapability) const;
	std::vector<Peer> GetAllPeers() const;
	std::vector<Peer> GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const;

	void AddFreshPeers(const std::vector<SocketAddress>& peerAddresses);
	void SetPeerConnected(const Peer& peer, const bool connected);
	void BanPeer(Peer& peer, const EBanReason banReason);
	// TODO: RemovePeer

private:
	struct PeerEntry
	{
		PeerEntry(Peer&& peer)
			: m_peer(std::move(peer)), m_lastAttempt(0), m_connected(false), m_dirty(false)
		{

		}

		PeerEntry(const Peer& peer, const time_t& lastAttempt, const bool connected, const bool dirty)
			: m_peer(peer), m_lastAttempt(lastAttempt), m_connected(connected), m_dirty(dirty)
		{

		}

		Peer m_peer;
		time_t m_lastAttempt;
		bool m_connected;
		bool m_dirty;
	};

	static void Thread_ManagePeers(PeerManager& peerManager);

	std::vector<Peer> GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers, const bool connectingToPeer) const;

	const Config& m_config;
	IPeerDB& m_peerDB;

	std::atomic_bool m_terminate;
	std::thread m_peerThread;

	mutable std::shared_mutex m_peersMutex;
	mutable std::unordered_set<IPAddress> m_peersServed;
	mutable std::unordered_map<IPAddress, PeerEntry> m_peersByAddress;
};