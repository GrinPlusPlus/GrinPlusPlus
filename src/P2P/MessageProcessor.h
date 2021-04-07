#pragma once

#include "Messages/RawMessage.h"
#include "Seed/PeerManager.h"

#include <BlockChain/BlockChain.h>
#include <P2P/ConnectedPeer.h>
#include <memory>

// Forward Declarations
class ConnectionManager;
class Connection;
class RawMessage;
class Pipeline;
class TxHashSetArchiveMessage;
class TxHashSetRequestMessage;

class MessageProcessor
{
public:
	enum EStatus
	{
		SUCCESS,
		SOCKET_FAILURE,
		UNKNOWN_ERROR,
		RESOURCE_NOT_FOUND,
		UNKNOWN_MESSAGE,
		SYNCING,
		DISCONNECT
	};

	MessageProcessor(
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		const IBlockChain::Ptr& pBlockChain,
		const std::shared_ptr<Pipeline>& pipeline,
		SyncStatusConstPtr pSyncStatus
	);

	void ProcessMessage(const std::shared_ptr<Connection>& pConnection, const RawMessage& rawMessage);

private:
	void ProcessMessageInternal(const std::shared_ptr<Connection>& pConnection, const RawMessage& rawMessage);

	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChain::Ptr m_pBlockChain;
	std::shared_ptr<Pipeline> m_pPipeline;
	SyncStatusConstPtr m_pSyncStatus;
};