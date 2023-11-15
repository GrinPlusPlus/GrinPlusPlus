#pragma once

#include <Net/Socket.h>
#include <P2P/SyncStatus.h>
#include <P2P/ConnectedPeer.h>

#include "../Messages/RawMessage.h"
#include "../Messages/PingMessage.h"
#include "../Messages/PongMessage.h"

class PingPong
{
public:
	PingPong(const SyncStatusConstPtr& pSyncStatus) : m_pSyncStatus(pSyncStatus) { }

	const PongMessage Execute(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const;

private:
	const PongMessage SendPing(const Socket::Ptr& pSocket) const;
	void TransmitPingMessage(const Socket::Ptr& pSocket) const;
	
	std::unique_ptr<RawMessage> RetrieveMessage(const Socket::Ptr& pSocket) const;

	SyncStatusConstPtr m_pSyncStatus;
};