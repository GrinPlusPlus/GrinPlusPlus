#pragma once

#include <Core/Config.h>
#include <P2P/SyncStatus.h>
#include <P2P/ConnectedPeer.h>

// Forward Declarations
class Socket;
class ConnectionManager;

class HandShake
{
public:
	HandShake(const Config& config, ConnectionManager& connectionManager, const SyncStatusConstPtr& pSyncStatus)
		: m_config(config), m_connectionManager(connectionManager), m_pSyncStatus(pSyncStatus) { }

	bool PerformHandshake(Socket& socket, ConnectedPeer& connectedPeer, const EDirection direction) const;

private:
	bool PerformOutboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const;
	bool PerformInboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const;
	bool TransmitHandMessage(Socket& socket) const;
	bool TransmitShakeMessage(Socket& socket, const uint32_t protocolVersion) const;

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	SyncStatusConstPtr m_pSyncStatus;
};