#pragma once

#include <Net/IPAddress.h>
#include <P2P/Peer.h>
#include <memory>
#include <optional>
#include <vector>

class IPeerDB
{
  public:
    virtual std::vector<Peer> LoadAllPeers() = 0;

    virtual std::optional<Peer> GetPeer(const IPAddress &address, const std::optional<uint16_t> &portOpt) = 0;

    virtual void SavePeers(const std::vector<Peer> &peers) = 0;
};