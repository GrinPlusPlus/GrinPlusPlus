#pragma once

#include "Connection.h"

#include <TxPool/TransactionPool.h>
#include <Config/Config.h>
#include <Common/ConcurrentQueue.h>
#include <memory>
#include <vector>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

// Forward Declarations
class PeerManager;

class ConnectionManager
{
public:
	static std::shared_ptr<ConnectionManager> Create(
		const Config& config,
		Locked<PeerManager> pPeerManager,
		ITransactionPoolPtr pTransactionPool
	);
	~ConnectionManager();

	void Shutdown();

	bool IsTerminating() const { return m_terminate; }

	void UpdateSyncStatus(SyncStatus& syncStatus) const;

	inline size_t GetNumOutbound() const { return m_numOutbound; }
	size_t GetNumberOfActiveConnections() const;
	std::pair<size_t, size_t> GetNumConnectionsWithDirection() const { return std::pair<size_t, size_t>(m_numInbound, m_numOutbound); }
	bool IsConnected(const uint64_t connectionId) const;
	bool IsConnected(const IPAddress& address) const;
	std::vector<uint64_t> GetMostWorkPeers() const;
	std::vector<ConnectedPeer> GetConnectedPeers() const;
	std::optional<std::pair<uint64_t, ConnectedPeer>> GetConnectedPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const;
	uint64_t GetMostWork() const;
	uint64_t GetHighestHeight() const;

	uint64_t SendMessageToMostWorkPeer(const IMessage& message);
	bool SendMessageToPeer(const IMessage& message, const uint64_t connectionId);
	void BroadcastMessage(const IMessage& message, const uint64_t sourceId);

	void PruneConnections(const bool bInactiveOnly);
	void AddConnection(ConnectionPtr pConnection);

	void BanConnection(const uint64_t connectionId, const EBanReason banReason);

private:
	ConnectionManager(const Config& config, ITransactionPoolPtr pTransactionPool);

	ConnectionPtr GetMostWorkPeer() const;
	ConnectionPtr GetConnectionById(const uint64_t connectionId) const;
	static void Thread_Broadcast(ConnectionManager& connectionManager);

	mutable std::shared_mutex m_connectionsMutex;
	std::vector<ConnectionPtr> m_connections;
	std::unordered_map<uint64_t, EBanReason> m_peersToBan;

	mutable std::shared_mutex m_disconnectMutex;
	std::vector<ConnectionPtr> m_connectionsToClose;

	struct MessageToBroadcast
	{
		MessageToBroadcast(uint64_t sourceId, std::shared_ptr<IMessage> pMessage)
			: m_sourceId(sourceId), m_pMessage(pMessage)
		{

		}
		uint64_t m_sourceId;
		std::shared_ptr<IMessage> m_pMessage;
	};

	ConcurrentQueue<MessageToBroadcast> m_sendQueue;
	std::thread m_broadcastThread;
	std::atomic_bool m_terminate;

	const Config& m_config;

	std::atomic<size_t> m_numOutbound;
	std::atomic<size_t> m_numInbound;
};

typedef std::shared_ptr<ConnectionManager> ConnectionManagerPtr;
typedef std::shared_ptr<const ConnectionManager> ConnectionManagerConstPtr;