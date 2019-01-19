#pragma once

#include "Connection.h"
#include "Sync/Syncer.h"
#include "Seed/Seeder.h"
#include "Pipeline.h"

#include <Config/Config.h>
#include <BlockChainServer.h>
#include <vector>
#include <shared_mutex>
#include <thread>
#include <set>

// Forward Declarations
class PeerManager;

class ConnectionManager
{
public:
	ConnectionManager(const Config& config, PeerManager& peerManager, IBlockChainServer& blockChainServer);

	void Start();
	void Stop();

	inline Pipeline& GetPipeline() { return m_pipeline; }
	inline const SyncStatus& GetSyncStatus() const { return m_syncer.GetSyncStatus(); }
	size_t GetNumberOfActiveConnections() const;
	std::vector<uint64_t> GetMostWorkPeers() const;
	uint64_t GetMostWork() const;
	uint64_t GetHighestHeight() const;

	uint64_t SendMessageToMostWorkPeer(const IMessage& message);
	bool SendMessageToPeer(const IMessage& message, const uint64_t connectionId);
	void BroadcastMessage(const IMessage& message, const uint64_t sourceId);

	void PruneConnections(const bool bInactiveOnly);
	void AddConnection(Connection* pConnection);

	void BanConnection(const uint64_t connectionId);

private:
	Connection* GetMostWorkPeer() const;
	Connection* GetConnectionById(const uint64_t connectionId) const;
	static void Thread_Broadcast(ConnectionManager& connectionManager);

	mutable std::shared_mutex m_connectionsMutex;
	std::vector<Connection*> m_connections;
	std::set<uint64_t> m_peersToBan;


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
};