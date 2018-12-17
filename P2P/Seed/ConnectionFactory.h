#pragma once

#include "../Connection.h"

#include <P2P/peer.h>
#include <WinSock2.h>
#include <memory>
#include <Config/Config.h>

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

	Connection* CreateConnection(Peer& peer) const;

private:
	Connection* PerformHandshake(SOCKET connection, Peer& peer) const;
	bool TransmitHandMessage(ConnectedPeer& connectedPeer) const;

private:
	const Config& m_config;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	IBlockChainServer& m_blockChainServer;
	mutable uint64_t m_nextId = { 1 };
};