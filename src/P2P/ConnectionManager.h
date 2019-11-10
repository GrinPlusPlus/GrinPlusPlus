#pragma once

#include "Connection.h"

#include <Common/ConcurrentQueue.h>
#include <Core/Traits/Lockable.h>
#include <memory>
#include <vector>
#include <thread>
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
	ConnectionManager();

	ConnectionPtr GetMostWorkPeer(const std::vector<ConnectionPtr>& connections) const;
	ConnectionPtr GetConnectionById(const uint64_t connectionId, const std::vector<ConnectionPtr>& connections) const;
	static void Thread_Broadcast(ConnectionManager& connectionManager);
	
	Locked<std::vector<ConnectionPtr>> m_connections;	
	Locked<std::unordered_map<uint64_t, EBanReason>> m_peersToBan;

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