#pragma once

#include <P2P/SyncStatus.h>
#include <P2P/ConnectedPeer.h>
#include "../Messages/RawMessage.h"

// Forward Declarations
class Socket;
class ConnectionManager;

class HandShake
{
public:
	HandShake(ConnectionManager& connectionManager, const SyncStatusConstPtr& pSyncStatus)
		: m_connectionManager(connectionManager), m_pSyncStatus(pSyncStatus) { }

	void PerformHandshake(Socket& socket, ConnectedPeer& connectedPeer) const;

private:
	void PerformOutboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const;
	void PerformInboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const;
	void TransmitHandMessage(Socket& socket) const;
	void TransmitShakeMessage(Socket& socket, const uint32_t protocolVersion) const;
	std::unique_ptr<RawMessage> RetrieveMessage(Socket& socket, const Peer& peer) const;

	ConnectionManager& m_connectionManager;
	SyncStatusConstPtr m_pSyncStatus;
};