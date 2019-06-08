#include "ConnectionManager.h"
#include "Seed/PeerManager.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PingMessage.h"

#include <thread>
#include <chrono>
#include <Common/Util/VectorUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>

ConnectionManager::ConnectionManager(const Config& config, PeerManager& peerManager, IBlockChainServer& blockChainServer, ITransactionPool& transactionPool)
	: m_config(config), 
	m_peerManager(peerManager), 
	m_blockChainServer(blockChainServer),
	m_syncer(*this, blockChainServer), 
	m_seeder(config, *this, peerManager, blockChainServer),
	m_pipeline(config, *this, blockChainServer),
	m_dandelion(config, *this, blockChainServer, transactionPool),
	m_numOutbound(0),
	m_numInbound(0)
{

}

void ConnectionManager::Start()
{
	m_terminate = false;

	m_peerManager.Start();
	m_seeder.Start();
	m_syncer.Start();
	m_pipeline.Start();
	m_dandelion.Start();

	if (m_broadcastThread.joinable())
	{
		m_broadcastThread.join();
	}

	m_broadcastThread = std::thread(Thread_Broadcast, std::ref(*this));
}

void ConnectionManager::Stop()
{
	m_terminate = true;

	try
	{
		m_seeder.Stop();
		m_syncer.Stop();
		m_pipeline.Stop();
		m_dandelion.Stop();

		if (m_broadcastThread.joinable())
		{
			m_broadcastThread.join();
		}

		PruneConnections(false);
		m_peerManager.Stop();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		LoggerAPI::LogError("ConnectionManager::Stop - " + std::string(e.what()));
	}
	catch (...)
	{
		LoggerAPI::LogError("ConnectionManager::Stop - Unknown exception thrown");
	}
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

std::pair<size_t, size_t> ConnectionManager::GetNumConnectionsWithDirection() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	size_t inbound = 0;
	size_t outbound = 0;
	for (Connection* pConnection : m_connections)
	{
		if (pConnection->GetConnectedPeer().GetDirection() == EDirection::INBOUND)
		{
			++inbound;
		}
		else
		{
			++outbound;
		}
	}

	return std::pair<size_t, size_t>(inbound, outbound);
}

bool ConnectionManager::IsConnected(const uint64_t connectionId) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	return GetConnectionById(connectionId) != nullptr;
}

bool ConnectionManager::IsConnected(const IPAddress& address) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	for (Connection* pConnection : m_connections)
	{
		if (pConnection->GetConnectedPeer().GetPeer().GetIPAddress() == address)
		{
			return pConnection->GetSocket().IsActive();
		}
	}

	return false;
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

std::vector<ConnectedPeer> ConnectionManager::GetConnectedPeers() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	std::vector<ConnectedPeer> connectedPeers;
	for (Connection* pConnection : m_connections)
	{
		connectedPeers.push_back(pConnection->GetConnectedPeer());
	}

	return connectedPeers;
}

std::optional<std::pair<uint64_t, ConnectedPeer>> ConnectionManager::GetConnectedPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	for (Connection* pConnection : m_connections)
	{
		ConnectedPeer connectedPeer = pConnection->GetConnectedPeer();
		if (connectedPeer.GetPeer().GetIPAddress() == address)
		{
			if (!portOpt.has_value() || portOpt.value() == connectedPeer.GetPeer().GetPortNumber() || !connectedPeer.GetPeer().GetIPAddress().IsLocalhost())
			{
				return std::make_optional<std::pair<uint64_t, ConnectedPeer>>(std::make_pair<uint64_t, ConnectedPeer>(pConnection->GetId(), std::move(connectedPeer)));
			}
		}
	}

	return std::nullopt;
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

	std::unique_lock<std::shared_mutex> disconnectLock(m_disconnectMutex);

	size_t numOutbound = 0;
	size_t numInbound = 0;
	for (int i = (int)m_connections.size() - 1; i >= 0; i--)
	{
		Connection* pConnection = m_connections[i];

		auto iter = m_peersToBan.find(pConnection->GetId());
		if (iter != m_peersToBan.end())
		{
			const EBanReason banReason = iter->second;
			LoggerAPI::LogWarning(StringUtil::Format("ConnectionManager::PruneConnections() - Banning peer (%d) at (%s) for (%d).", pConnection->GetId(), pConnection->GetPeer().GetIPAddress().Format().c_str(), (int32_t)banReason));

			pConnection->GetPeer().UpdateLastBanTime();
			pConnection->GetPeer().UpdateBanReason(banReason);

			m_connectionsToClose.push_back(pConnection);
			VectorUtil::Remove<Connection*>(m_connections, i);

			continue;
		}

		if (!bInactiveOnly || !pConnection->IsConnectionActive())
		{
			if (!pConnection->IsConnectionActive())
			{
				LoggerAPI::LogDebug(StringUtil::Format("ConnectionManager::PruneConnections() - Disconnecting from inactive peer (%d) at (%s)", pConnection->GetId(), pConnection->GetPeer().GetIPAddress().Format().c_str()));
			}

			m_connectionsToClose.push_back(pConnection);
			VectorUtil::Remove<Connection*>(m_connections, i);

			continue;
		}

		if (pConnection->GetConnectedPeer().GetDirection() == EDirection::INBOUND)
		{
			numInbound++;
		}
		else
		{
			numOutbound++;
		}
	}

	m_numInbound = numInbound;
	m_numOutbound = numOutbound;
	m_peersToBan.clear();
	writeLock.unlock();

	for (Connection* pConnection : m_connectionsToClose)
	{
		try
		{
			pConnection->Disconnect();
			delete pConnection;
		}
		catch (std::exception& e)
		{
			LoggerAPI::LogError("ConnectionManager::PruneConnections - Error disconnecting: " + std::string(e.what()));
		}
	}

	m_connectionsToClose.clear();
}

void ConnectionManager::BanConnection(const uint64_t connectionId, const EBanReason banReason)
{
	std::unique_lock<std::shared_mutex> writeLock(m_connectionsMutex);

	m_peersToBan.insert(std::pair<uint64_t, EBanReason>(connectionId, banReason));
}

Connection* ConnectionManager::GetMostWorkPeer() const
{
	std::vector<Connection*> mostWorkPeers;
	uint64_t mostWork = 0;
	uint64_t mostWorkHeight = 0;
	for (Connection* pConnection : m_connections)
	{
		if (pConnection->GetHeight() == 0)
		{
			continue;
		}

		if (pConnection->GetTotalDifficulty() > mostWork)
		{
			mostWork = pConnection->GetTotalDifficulty();
			mostWorkHeight = pConnection->GetHeight();
			mostWorkPeers.clear();
			mostWorkPeers.emplace_back(pConnection);
		}
		else if (pConnection->GetTotalDifficulty() == mostWork)
		{
			if (pConnection->GetHeight() > mostWorkHeight)
			{
				mostWorkHeight = pConnection->GetHeight();
				mostWorkPeers.clear();
				mostWorkPeers.emplace_back(pConnection);
			}
			else if (pConnection->GetHeight() == mostWorkHeight)
			{
				mostWorkPeers.emplace_back(pConnection);
			}
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
			ThreadUtil::SleepFor(std::chrono::milliseconds(100), connectionManager.m_terminate);
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