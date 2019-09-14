#pragma once

#include "Messages/RawMessage.h"

#include <P2P/ConnectedPeer.h>
#include <Config/Config.h>

// Forward Declarations
class Socket;
class ConnectionManager;
class IBlockChainServer;
class ConnectedPeer;
class RawMessage;
class TxHashSetArchiveMessage;
class TxHashSetRequestMessage;
class PeerManager;

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

	MessageProcessor(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer);

	EStatus ProcessMessage(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const RawMessage& rawMessage);

private:
	EStatus ProcessMessageInternal(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const RawMessage& rawMessage);
	EStatus SendTxHashSet(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const TxHashSetRequestMessage& txHashSetRequestMessage);
	EStatus ReceiveTxHashSet(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const TxHashSetArchiveMessage& txHashSetArchiveMessage);

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	IBlockChainServer& m_blockChainServer;
};