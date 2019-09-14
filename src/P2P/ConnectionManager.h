#pragma once

#include "Connection.h"
#include "Sync/Syncer.h"
#include "Seed/Seeder.h"
#include "Pipeline.h"
#include "Dandelion.h"

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <vector>
#include <shared_mutex>
#include <thread>
#include <set>
#include <unordered_map>

// Forward Declarations
class PeerManager;

class ConnectionManager
{
public:
	ConnectionManager(const Config& config, PeerManager& peerManager, IBlockChainServer& blockChainServer, ITransactionPool& transactionPool);

	void Start();
	void Stop();

	inline Pipeline& GetPipeline() { return m_pipeline; }
	inline bool IsTerminating() const { return m_terminate; }

	inline SyncStatus& GetSyncStatus() { return m_syncer.GetSyncStatus(); }
	inline const SyncStatus& GetSyncStatus() const { return m_syncer.GetSyncStatus(); }
	void UpdateSyncStatus(SyncStatus& syncStatus) const;

	inline size_t GetNumOutbound() const { return m_numOutbound; }
	size_t GetNumberOfActiveConnections() const;
	std::pair<size_t, size_t> GetNumConnectionsWithDirection() const;
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
	void AddConnection(Connection* pConnection);

	void BanConnection(const uint64_t connectionId, const EBanReason banReason);

private:
	Connection* GetMostWorkPeer() const;
	Connection* GetConnectionById(const uint64_t connectionId) const;
	static void Thread_Broadcast(ConnectionManager& connectionManager);

	mutable std::shared_mutex m_connectionsMutex;
	std::vector<Connection*> m_connections;
	std::unordered_map<uint64_t, EBanReason> m_peersToBan;

	mutable std::shared_mutex m_disconnectMutex;
	std::vector<Connection*> m_connectionsToClose;

	struct MessageToBroadcast
	{
		MessageToBroadcast(uint64_t sourceId, IMessage* pMessage)
			: m_sourceId(sourceId), m_pMessage(pMessage)
		{

		}
		uint64_t m_sourceId;
		IMessage* m_pMessage;
	};
	mutable std::mutex m_broadcastMutex;
	std::queue<MessageToBroadcast> m_sendQueue;
	std::thread m_broadcastThread;
	std::atomic_bool m_terminate;

	const Config& m_config;
	PeerManager& m_peerManager;
	IBlockChainServer& m_blockChainServer;
	Syncer m_syncer;
	Seeder m_seeder;
	Pipeline m_pipeline;
	Dandelion m_dandelion;

	std::atomic<size_t> m_numOutbound;
	std::atomic<size_t> m_numInbound;
};