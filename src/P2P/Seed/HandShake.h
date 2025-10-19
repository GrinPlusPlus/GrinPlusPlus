#pragma once

#include <Net/Socket.h>
#include <P2P/SyncStatus.h>
#include <P2P/ConnectedPeer.h>
#include <P2P/Capabilities.h>
#include <P2P/Direction.h>

#include "../Messages/RawMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/HandMessage.h"


class HandShake
{
public:
	HandShake(const SyncStatusConstPtr& pSyncStatus,
			  const Socket::Ptr& pSocket) : m_pSyncStatus(pSyncStatus),
											m_pSocket(pSocket),
											m_TotalDifficulty(0) { }
	void Perform(ConnectedPeer& connectedPeer, EDirection direction);

	ShakeMessage RetrieveShakeMessage();
	HandMessage RetrieveHandMessage();

private:
	void TransmitHandMessage() ;
	void TransmitShakeMessage();

	std::unique_ptr<RawMessage> RetrieveMessage();

	void UpdateConnectedPeer(ConnectedPeer& connectedPeer);

	uint32_t m_Version = 0;
	Capabilities m_Capabilities;
	std::string m_UserAgent = "";
	uint64_t m_TotalDifficulty = 0;

	const Socket::Ptr& m_pSocket;
	
	const SyncStatusConstPtr m_pSyncStatus;
};