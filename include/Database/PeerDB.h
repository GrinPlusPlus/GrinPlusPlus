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

	virtual std::vector<PeerPtr> LoadAllPeers() const = 0;

	virtual std::optional<PeerPtr> GetPeer(
		const IPAddress& address,
		const std::optional<uint16_t>& portOpt
	) const = 0;

	virtual void SavePeers(const std::vector<PeerPtr>& peers) = 0;
	virtual void DeletePeers(const std::vector<PeerPtr>& peers) = 0;
};