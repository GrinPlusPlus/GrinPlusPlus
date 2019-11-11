#pragma once

#include <P2P/Peer.h>
#include <Net/IPAddress.h>
#include <Core/Traits/Lockable.h>
#include <Core/Traits/Batchable.h>
#include <memory>
#include <vector>
#include <optional>

class IPeerDB : public Traits::IBatchable
{
public:
	virtual ~IPeerDB() = default;

	virtual std::vector<Peer> LoadAllPeers() const = 0;

	virtual std::optional<Peer> GetPeer(
		const IPAddress& address,
		const std::optional<uint16_t>& portOpt
	) const = 0;

	virtual void SavePeers(const std::vector<Peer>& peers) = 0;
};