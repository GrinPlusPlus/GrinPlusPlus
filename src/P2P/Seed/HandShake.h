#pragma once

#include <Net/Socket.h>
#include <P2P/SyncStatus.h>
#include <P2P/ConnectedPeer.h>
#include "../Messages/RawMessage.h"

// Forward Declarations
class ConnectionManager;

class HandShake
{
public:
	HandShake(ConnectionManager& connectionManager, const SyncStatusConstPtr& pSyncStatus)
		: m_connectionManager(connectionManager), m_pSyncStatus(pSyncStatus) { }

	void PerformHandshake(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const;

private:
	void PerformOutboundHandshake(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const;
	void PerformInboundHandshake(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const;
	void TransmitHandMessage(const Socket::Ptr& pSocket) const;
	void TransmitShakeMessage(const Socket::Ptr& pSocket, const uint32_t protocolVersion) const;
	std::unique_ptr<RawMessage> RetrieveMessage(const Socket::Ptr& pSocket, const Peer& peer) const;

	ConnectionManager& m_connectionManager;
	SyncStatusConstPtr m_pSyncStatus;
};