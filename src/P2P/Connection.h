#pragma once

#include "Seed/PeerManager.h"
#include "Messages/Message.h"

#include <BlockChain/BlockChainServer.h>
#include <Net/Socket.h>
#include <P2P/ConnectedPeer.h>
#include <Config/Config.h>
#include <mutex>
#include <atomic>
#include <queue>

// Forward Declarations
class IMessage;
class ConnectionManager;
class PeerManager;
class Pipeline;

//
// A Connection will be created for each ConnectedPeer.
// Each Connection will run on its own thread, and will watch the socket for messages,
// and will ping the peer when it hasn't been heard from in a while.
//
class Connection
{
public:
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection(Connection&&) = delete;
	~Connection();

	static std::shared_ptr<Connection> Create(
		SocketPtr pSocket,
		const uint64_t connectionId,
		const Config& config,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		const ConnectedPeer& connectedPeer,
		std::shared_ptr<Pipeline> pPipeline,
		SyncStatusConstPtr pSyncStatus
	);

	void Disconnect();

	inline uint64_t GetId() const { return m_connectionId; }
	bool IsConnectionActive() const;

	void Send(const IMessage& message);

	inline SocketPtr GetSocket() const { return m_pSocket; }
	inline Peer& GetPeer() { return m_connectedPeer.GetPeer(); }
	inline const Peer& GetPeer() const { return m_connectedPeer.GetPeer(); }
	inline const ConnectedPeer& GetConnectedPeer() const { return m_connectedPeer; }
	inline uint64_t GetTotalDifficulty() const { return m_connectedPeer.GetTotalDifficulty(); }
	inline uint64_t GetHeight() const { return m_connectedPeer.GetHeight(); }
	inline Capabilities GetCapabilities() const { return m_connectedPeer.GetPeer().GetCapabilities(); }

	bool ExceedsRateLimit() const;

private:
	Connection(
		SocketPtr pSocket,
		const uint64_t connectionId,
		const Config& config,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		const ConnectedPeer& connectedPeer,
		Pipeline& pipeline,
		SyncStatusConstPtr pSyncStatus
	);

	static void Thread_ProcessConnection(std::shared_ptr<Connection> pConnection);

	const Config& m_config;
	IBlockChainServerPtr m_pBlockChainServer;
	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	Pipeline& m_pipeline;
	SyncStatusConstPtr m_pSyncStatus;

	std::atomic<bool> m_terminate = true;
	std::thread m_connectionThread;
	const uint64_t m_connectionId;

	mutable std::mutex m_peerMutex;
	ConnectedPeer m_connectedPeer;

	std::shared_ptr<asio::io_context> m_pContext;
	mutable SocketPtr m_pSocket;

	mutable std::mutex m_sendMutex;
	std::queue<IMessagePtr> m_sendQueue;
};

typedef std::shared_ptr<Connection> ConnectionPtr;