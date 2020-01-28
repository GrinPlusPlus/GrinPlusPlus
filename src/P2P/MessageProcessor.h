#pragma once

#include "Messages/RawMessage.h"
#include "Seed/PeerManager.h"

#include <BlockChain/BlockChainServer.h>
#include <P2P/ConnectedPeer.h>
#include <Config/Config.h>
#include <memory>

// Forward Declarations
class Socket;
class ConnectionManager;
class ConnectedPeer;
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
		BAN_PEER
	};

	MessageProcessor(
		const Config& config,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		const std::shared_ptr<Pipeline>& pipeline,
		SyncStatusConstPtr pSyncStatus
	);

	EStatus ProcessMessage(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const RawMessage& rawMessage);

private:
	EStatus ProcessMessageInternal(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const RawMessage& rawMessage);
	EStatus SendTxHashSet(ConnectedPeer& connectedPeer, Socket& socket, const TxHashSetRequestMessage& txHashSetRequestMessage);

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChainServerPtr m_pBlockChainServer;
	std::shared_ptr<Pipeline> m_pPipeline;
	SyncStatusConstPtr m_pSyncStatus;
};