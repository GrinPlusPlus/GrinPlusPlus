#pragma once

#include <P2P/Peer.h>
#include <P2P/IPAddress.h>
#include <memory>
#include <vector>
#include <optional>

class IPeerDB
{
public:
	virtual std::vector<Peer> LoadAllPeers() = 0;
	virtual std::optional<Peer> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) = 0;

	virtual void SavePeers(const std::vector<Peer>& peers) = 0;
};