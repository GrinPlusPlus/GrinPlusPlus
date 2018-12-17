#pragma once

#include <P2P/IPAddress.h>
#include <P2P/peer.h>
#include <P2P/SocketAddress.h>
#include <P2P/capabilities.h>
#include <Database/PeerDB.h>
#include <Config/Config.h>

#include <map>
#include <set>
#include <memory>
#include <shared_mutex>

class PeerManager
{
public:
	PeerManager(const Config& config, IPeerDB& peerDB);
	
	bool ArePeersNeeded(const Capabilities::ECapability& preferredCapability) const;
	std::unique_ptr<Peer> GetNewPeer(const Capabilities::ECapability& preferredCapability) const;
	std::vector<Peer> GetPeers(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const;
	void AddPeerAddresses(const std::vector<SocketAddress>& peerAddresses);
	bool UpdatePeer(const Peer& peer);
	// TODO: BanPeer & RemovePeer

private:
	bool AddPeer(const Peer& peer);
	std::vector<Peer> GetPeersWithCapability(const Capabilities::ECapability& preferredCapability, const uint16_t maxPeers) const;

	const Config& m_config;
	IPeerDB& m_peerDB;

	mutable std::shared_mutex m_peersMutex;
	mutable std::set<IPAddress> m_peersServed;
	mutable std::map<IPAddress, Peer> m_peersByAddress;
};