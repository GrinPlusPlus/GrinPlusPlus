#include "ConnectionManager.h"
#include "Seed/PeerManager.h"
#include "Messages/GetPeerAddressesMessage.h"

#include <thread>
#include <chrono>
#include <VectorUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <StringUtil.h>

ConnectionManager::ConnectionManager(const Config& config, PeerManager& peerManager, IBlockChainServer& blockChainServer)
	: m_config(config), 
	m_peerManager(peerManager), 
	m_blockChainServer(blockChainServer),
	m_syncer(*this, blockChainServer), 
	m_seeder(config, *this, peerManager, blockChainServer),
	m_pipeline(config, *this, blockChainServer)
{

}

void ConnectionManager::Start()
{
	m_seeder.Start();
	m_syncer.Start();
	m_pipeline.Start();

	m_terminate = false;

	if (m_broadcastThread.joinable())
	{
		m_broadcastThread.join();
	}

	m_broadcastThread = std::thread(Thread_Broadcast, std::ref(*this));
}

void ConnectionManager::Stop()
{
	m_seeder.Stop();
	m_syncer.Stop();
	m_pipeline.Stop();

	m_terminate = true;

	if (m_broadcastThread.joinable())
	{
		m_broadcastThread.join();
	}

	PruneConnections(false);
}

void ConnectionManager::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	const uint64_t numActiveConnections = m_connections.size();
	Connection* pMostWorkPeer = GetMostWorkPeer();
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t networkHeight = pMostWorkPeer->GetHeight();
		const uint64_t networkDifficulty = pMostWorkPeer->GetTotalDifficulty();
		syncStatus.UpdateNetworkStatus(numActiveConnections, networkHeight, networkDifficulty);
	}
}

size_t ConnectionManager::GetNumberOfActiveConnections() const
{
	return m_connections.size();
}

std::vector<uint64_t> ConnectionManager::GetMostWorkPeers() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	std::vector<uint64_t> mostWorkPeers;

	Connection* pMostWorkPeer = GetMostWorkPeer();
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t totalDifficulty = pMostWorkPeer->GetTotalDifficulty();
		for (Connection* pConnection : m_connections)
		{
			if (pConnection->GetTotalDifficulty() >= totalDifficulty && pConnection->GetHeight() > 0)
			{
				mostWorkPeers.push_back(pConnection->GetId());
			}
		}
	}

	return mostWorkPeers;
}

uint64_t ConnectionManager::GetMostWork() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	Connection* pConnection = GetMostWorkPeer();
	if (pConnection != nullptr)
	{
		return pConnection->GetTotalDifficulty();
	}

	return 0;
}

uint64_t ConnectionManager::GetHighestHeight() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	Connection* pConnection = GetMostWorkPeer();
	if (pConnection != nullptr)
	{
		return pConnection->GetHeight();
	}

	return 0;
}

uint64_t ConnectionManager::SendMessageToMostWorkPeer(const IMessage& message)
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	Connection* pConnection = GetMostWorkPeer();
	if (pConnection != nullptr)
	{
		pConnection->Send(message);
		return pConnection->GetId();
	}

	return 0;
}

bool ConnectionManager::SendMessageToPeer(const IMessage& message, const uint64_t connectionId)
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	Connection* pConnection = GetConnectionById(connectionId);
	if (pConnection != nullptr)
	{
		pConnection->Send(message);
		return true;
	}

	return false;
}

void ConnectionManager::BroadcastMessage(const IMessage& message, const uint64_t sourceId)
{
	std::unique_lock<std::mutex> writeLock(m_broadcastMutex);
	m_sendQueue.emplace(MessageToBroadcast(sourceId, message.Clone()));
}

void ConnectionManager::AddConnection(Connection* pConnection)
{
	std::unique_lock<std::shared_mutex> writeLock(m_connectionsMutex);

	m_connections.emplace_back(pConnection);
}

void ConnectionManager::PruneConnections(const bool bInactiveOnly)
{
	std::unique_lock<std::shared_mutex> writeLock(m_connectionsMutex);
	for (int i = (int)m_connections.size() - 1; i >= 0; i--)
	{
		Connection* pConnection = m_connections[i];

		if (m_peersToBan.count(pConnection->GetId()) > 0)
		{
			// TODO: Implement this.
			LoggerAPI::LogWarning(StringUtil::Format("ConnectionManager::BanConnection() - Banning peer (%d) at (%s).", pConnection->GetId(), pConnection->GetPeer().GetIPAddress().Format().c_str()));
			pConnection->Disconnect();

			VectorUtil::Remove<Connection*>(m_connections, i);
			delete pConnection;

			continue;
		}

		if (!bInactiveOnly || !pConnection->IsConnectionActive())
		{
			pConnection->Disconnect();

			m_peerManager.UpdatePeer(pConnection->GetPeer()); // TODO: Also save connection stats.

			VectorUtil::Remove<Connection*>(m_connections, i);
			delete pConnection;
		}
	}
}

void ConnectionManager::BanConnection(const uint64_t connectionId)
{
	std::unique_lock<std::shared_mutex> writeLock(m_connectionsMutex);

	m_peersToBan.insert(connectionId);
}

Connection* ConnectionManager::GetMostWorkPeer() const
{
	std::vector<Connection*> mostWorkPeers;
	uint64_t mostWork = 0;
	for (Connection* pConnection : m_connections)
	{
		if (pConnection->GetHeight() == 0)
		{
			continue;
		}

		if (pConnection->GetTotalDifficulty() > mostWork)
		{
			mostWork = pConnection->GetTotalDifficulty();
			mostWorkPeers.clear();
			mostWorkPeers.emplace_back(pConnection);
		}
		else if (pConnection->GetTotalDifficulty() == mostWork)
		{
			mostWorkPeers.emplace_back(pConnection);
		}
	}

	if (mostWorkPeers.empty())
	{
		return nullptr;
	}

	const int index = std::rand() % mostWorkPeers.size();

	return mostWorkPeers[index];
}

Connection* ConnectionManager::GetConnectionById(const uint64_t connectionId) const
{
	for (Connection* pConnection : m_connections)
	{
		if (pConnection->GetId() == connectionId)
		{
			return pConnection;
		}
	}

	return nullptr;
}

void ConnectionManager::Thread_Broadcast(ConnectionManager& connectionManager)
{
	while (!connectionManager.m_terminate) 
	{
		std::unique_lock<std::mutex> messageWriteLock(connectionManager.m_broadcastMutex);
		if (connectionManager.m_sendQueue.empty())
		{
			messageWriteLock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		MessageToBroadcast broadcastMessage(connectionManager.m_sendQueue.front());
		connectionManager.m_sendQueue.pop();
		messageWriteLock.unlock();

		std::shared_lock<std::shared_mutex> readLock(connectionManager.m_connectionsMutex);
		// TODO: This should only broadcast to 8(?) peers. Should maybe be configurable.
		for (Connection* pConnection : connectionManager.m_connections)
		{
			if (pConnection->GetId() != broadcastMessage.m_sourceId)
			{
				pConnection->Send(*broadcastMessage.m_pMessage);
			}
		}

		delete broadcastMessage.m_pMessage;
	}
}