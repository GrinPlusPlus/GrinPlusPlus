#pragma once

#include "Messages/Message.h"

#include <caches/Cache.h>
#include <Core/Enums/ProtocolVersion.h>
#include <Net/Socket.h>
#include <P2P/ConnectedPeer.h>
#include <P2P/SyncStatus.h>
#include <Core/Config.h>
#include <chrono>
#include <atomic>
#include <queue>

// Forward Declarations
class IMessage;
class ConnectionManager;
class MessageProcessor;
class MessageRetriever;

//
// A Connection will be created for each ConnectedPeer.
// Each Connection will run on its own thread, and will watch the socket for messages,
// and will ping the peer when it hasn't been heard from in a while.
//
class Connection : public Traits::IPrintable, public std::enable_shared_from_this<Connection>
{
	using system_clock = std::chrono::system_clock;

public:
	using Ptr = std::shared_ptr<Connection>;

	Connection(
		const SocketPtr& pSocket,
		const uint64_t connectionId,
		ConnectionManager& connectionManager,
		const ConnectedPeer& connectedPeer,
		SyncStatusConstPtr pSyncStatus,
		const std::weak_ptr<MessageProcessor>& pMessageProcessor
	)	: m_pSocket(pSocket),
		m_connectionId(connectionId),
		m_connectionManager(connectionManager),
		m_connectedPeer(connectedPeer),
		m_pSyncStatus(pSyncStatus),
		m_pMessageProcessor(pMessageProcessor),
		m_terminate(false),
		m_sendingDisabled(false),
		m_advertisedBlocks(256) { }

	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection(Connection&&) = delete;
	~Connection() { Disconnect(); }

	static Connection::Ptr CreateInbound(
		const PeerPtr& pPeer,
		const SocketPtr& pSocket,
		const uint64_t connectionId,
		ConnectionManager& connectionManager,
		const std::weak_ptr<MessageProcessor>& pMessageProcessor,
		const SyncStatusConstPtr& pSyncStatus
	);

	static Connection::Ptr CreateOutbound(
		const PeerPtr& pPeer,
		const uint64_t connectionId,
		const std::shared_ptr<asio::io_service>& pAsioContext,
		ConnectionManager& connectionManager,
		const std::weak_ptr<MessageProcessor>& pMessageProcessor,
		const SyncStatusConstPtr& pSyncStatus
	);

	void Disconnect();

	uint64_t GetId() const { return m_connectionId; }
	bool IsConnectionActive() const;

	void DisableSends(bool disabled) { m_sendingDisabled = disabled; }
	void SendAsync(const IMessage& message);
	bool SendSync(const IMessage& message);
	bool ExceedsRateLimit() const;
	void BanPeer(const EBanReason reason);
	void CheckPing();

	SocketPtr GetSocket() const { return m_pSocket; }
	PeerPtr GetPeer() { return m_connectedPeer.GetPeer(); }
	PeerConstPtr GetPeer() const { return m_connectedPeer.GetPeer(); }
	const ConnectedPeer& GetConnectedPeer() const { return m_connectedPeer; }
	const IPAddress& GetIPAddress() const { return GetPeer()->GetIPAddress(); }
	uint64_t GetTotalDifficulty() const { return m_connectedPeer.GetTotalDifficulty(); }
	uint64_t GetHeight() const { return m_connectedPeer.GetHeight(); }
	Capabilities GetCapabilities() const { return m_connectedPeer.GetPeer()->GetCapabilities(); }
	EProtocolVersion GetProtocolVersion() const noexcept { return ProtocolVersion::ToEnum(GetPeer()->GetVersion()); }
	const EDirection GetDirection() const noexcept { return m_connectedPeer.GetDirection(); }
	void UpdateTotals(const uint64_t totalDifficulty, const uint64_t height) { m_connectedPeer.UpdateTotals(totalDifficulty, height); }

	std::string Format() const final { return "Connection{" + GetIPAddress().Format() + "}"; }

	bool HasBlock(const Hash& hash) const { return m_advertisedBlocks.Cached(hash); }
	void AdvertisedBlock(const Hash& hash) { return m_advertisedBlocks.Put(hash, hash); }

private:
	static void Thread_ProcessConnection(std::shared_ptr<Connection> pConnection);
	void HandleConnected(const asio::error_code& ec);
	void HandleReceived(const asio::error_code& ec, const size_t bytes_received);

	void ConnectOutbound();

	ConnectionManager& m_connectionManager;
	SyncStatusConstPtr m_pSyncStatus;
	std::weak_ptr<MessageProcessor> m_pMessageProcessor;

	std::chrono::system_clock::time_point m_lastPing;
	std::chrono::system_clock::time_point m_lastReceived;
	std::vector<uint8_t> m_received;

	std::atomic<bool> m_terminate;
	std::atomic<bool> m_sendingDisabled;
	std::thread m_connectionThread;
	const uint64_t m_connectionId;
	ConnectedPeer m_connectedPeer;
	mutable SocketPtr m_pSocket;

	mutable LRUCache<Hash, Hash> m_advertisedBlocks;
	mutable std::mutex m_mutex;
};

typedef std::shared_ptr<Connection> ConnectionPtr;