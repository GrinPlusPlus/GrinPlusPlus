#pragma once

#include "Messages/RawMessage.h"
#include "Seed/PeerManager.h"

#include <BlockChain/BlockChain.h>
#include <P2P/ConnectedPeer.h>
#include <Core/Config.h>
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
		const Config& config,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		const IBlockChain::Ptr& pBlockChain,
		const std::shared_ptr<Pipeline>& pipeline,
		SyncStatusConstPtr pSyncStatus
	);

	void ProcessMessage(Connection& connection, const RawMessage& rawMessage);

private:
	void ProcessMessageInternal(Connection& connection, const RawMessage& rawMessage);
	void SendTxHashSet(Connection& connection, const TxHashSetRequestMessage& txHashSetRequestMessage);

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChain::Ptr m_pBlockChain;
	std::shared_ptr<Pipeline> m_pPipeline;
	SyncStatusConstPtr m_pSyncStatus;
};