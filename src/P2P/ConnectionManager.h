#pragma once

#include "Connection.h"

#include <Common/ConcurrentQueue.h>
#include <Core/Traits/Lockable.h>
#include <memory>
#include <vector>
#include <thread>
#include <optional>
#include <unordered_map>

class ConnectionManager
{
public:
	static std::shared_ptr<ConnectionManager> Create();
	~ConnectionManager();

	void Shutdown();

	void UpdateSyncStatus(SyncStatus& syncStatus) const;

	size_t GetNumInbound() const { return m_numInbound; }
	size_t GetNumOutbound() const { return m_numOutbound; }
	size_t GetNumberOfActiveConnections() const { return m_connections.Read()->size(); }

	bool IsConnected(const IPAddress& address) const;
	std::vector<PeerPtr> GetMostWorkPeers() const;
	std::vector<ConnectedPeer> GetConnectedPeers() const;
	uint64_t GetMostWork() const;
	uint64_t GetHighestHeight() const;

	PeerPtr SendMessageToMostWorkPeer(const IMessage& message);
	bool SendMessageToPeer(const IMessage& message, PeerConstPtr pPeer);
	void BroadcastMessage(const IMessage& message, const uint64_t sourceId);

	void PruneConnections(const bool bInactiveOnly);
	void AddConnection(ConnectionPtr pConnection);
	ConnectionPtr GetConnection(const uint64_t connectionId) const;

private:
	ConnectionManager();

	ConnectionPtr GetMostWorkPeer(const std::vector<ConnectionPtr>& connections) const;
	static void Thread_Broadcast(ConnectionManager& connectionManager);
	
	Locked<std::vector<ConnectionPtr>> m_connections;

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

	std::atomic<size_t> m_numOutbound;
	std::atomic<size_t> m_numInbound;
};

typedef std::shared_ptr<ConnectionManager> ConnectionManagerPtr;
typedef std::shared_ptr<const ConnectionManager> ConnectionManagerConstPtr;