#pragma once

#include "../Connection.h"

#include <P2P/peer.h>
#include <P2P/Direction.h>
#include <Net/Socket.h>
#include <memory>
#include <Config/Config.h>
#include <optional>

// Forward Declarations
class Peer;
class ConnectedPeer;
class PeerManager;
class IBlockChainServer;
class ConnectionManager;

class ConnectionFactory
{
public:
	ConnectionFactory(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer);

	Connection* CreateConnection(Peer& peer, const EDirection direction, const std::optional<Socket>& socketOpt) const;

private:
	Connection* PerformHandshake(Socket& connection, Peer& peer, const EDirection direction) const;
	bool TransmitHandMessage(ConnectedPeer& connectedPeer) const;
	bool TransmitShakeMessage(ConnectedPeer& connectedPeer) const;

private:
	const Config& m_config;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	IBlockChainServer& m_blockChainServer;
	mutable uint64_t m_nextId = { 1 };
};