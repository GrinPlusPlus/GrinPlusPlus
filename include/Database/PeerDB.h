#pragma once

#include <P2P/Peer.h>
#include <P2P/IPAddress.h>
#include <memory>
#include <vector>

class IPeerDB
{
public:
	virtual std::vector<Peer> LoadAllPeers() = 0;
	virtual std::unique_ptr<Peer> GetPeer(const IPAddress& address) = 0;

	virtual void AddPeers(const std::vector<Peer>& peers) = 0;
};