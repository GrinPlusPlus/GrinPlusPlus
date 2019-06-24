#pragma once

#include "Messages/Message.h"
#include "Socket.h"

#include <P2P/ConnectedPeer.h>
#include <Config/Config.h>
#include <mutex>
#include <atomic>
#include <queue>

// Forward Declarations
class IMessage;
class IBlockChainServer;
class ConnectionManager;
class PeerManager;

//
// A Connection will be created for each ConnectedPeer.
// Each Connection will run on its own thread, and will watch the socket for messages,
// and will ping the peer when it hasn't been heard from in a while.
//
class Connection
{
public:
	Connection(Socket&& socket, const uint64_t connectionId, const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer, const ConnectedPeer& connectedPeer);

	inline uint64_t GetId() const { return m_connectionId; }
	bool Connect();
	bool IsConnectionActive() const;
	void Disconnect();

	void Send(const IMessage& message);

	inline Socket& GetSocket() const { return m_socket; }
	inline Peer& GetPeer() { return m_connectedPeer.GetPeer(); }
	inline const Peer& GetPeer() const { return m_connectedPeer.GetPeer(); }
	inline const ConnectedPeer& GetConnectedPeer() const { return m_connectedPeer; }
	inline uint64_t GetTotalDifficulty() const { return m_connectedPeer.GetTotalDifficulty(); }
	inline uint64_t GetHeight() const { return m_connectedPeer.GetHeight(); }
	inline Capabilities GetCapabilities() const { return m_connectedPeer.GetPeer().GetCapabilities(); }

private:
	static void Thread_ProcessConnection(Connection* pConnection);

	const Config& m_config;
	IBlockChainServer& m_blockChainServer;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	std::atomic<bool> m_terminate = true;
	std::thread m_connectionThread;
	const uint64_t m_connectionId;

	mutable std::mutex m_peerMutex;
	ConnectedPeer m_connectedPeer;

	asio::io_context m_context;
	mutable Socket m_socket;

	mutable std::mutex m_sendMutex;
	std::queue<IMessage*> m_sendQueue;
};