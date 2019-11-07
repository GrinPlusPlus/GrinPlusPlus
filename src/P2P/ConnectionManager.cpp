#include "ConnectionManager.h"
#include "Sync/Syncer.h"
#include "Pipeline/Pipeline.h"
#include "Seed/PeerManager.h"
#include "Seed/Seeder.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PingMessage.h"

#include <thread>
#include <chrono>
#include <Common/Util/VectorUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>

ConnectionManager::ConnectionManager(
	const Config& config,
	ITransactionPoolPtr pTransactionPool)
	: m_config(config), 
	m_numOutbound(0),
	m_numInbound(0),
	m_terminate(false)
{

}

ConnectionManager::~ConnectionManager()
{
	Shutdown();
}

void ConnectionManager::Shutdown()
{
	if (!m_terminate)
	{
		m_terminate = true;

		try
		{
			ThreadUtil::Join(m_broadcastThread);

			PruneConnections(false);
		}
		catch (const std::exception& e)
		{
			LOG_ERROR(std::string(e.what()));
		}
		catch (...)
		{
			LOG_ERROR("Unknown exception thrown");
		}
	}
}

std::shared_ptr<ConnectionManager> ConnectionManager::Create(
	const Config& config,
	Locked<PeerManager> pPeerManager,
	ITransactionPoolPtr pTransactionPool)
{
	auto pConnectionManager = std::shared_ptr<ConnectionManager>(new ConnectionManager(config, pTransactionPool));
	pConnectionManager->m_broadcastThread = std::thread(Thread_Broadcast, std::ref(*pConnectionManager.get()));
	return pConnectionManager;
}

void ConnectionManager::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	const uint64_t numActiveConnections = m_connections.size();
	ConnectionPtr pMostWorkPeer = GetMostWorkPeer();
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

bool ConnectionManager::IsConnected(const uint64_t connectionId) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	return GetConnectionById(connectionId) != nullptr;
}

bool ConnectionManager::IsConnected(const IPAddress& address) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	for (ConnectionPtr pConnection : m_connections)
	{
		if (pConnection->GetConnectedPeer().GetPeer().GetIPAddress() == address)
		{
			return pConnection->IsConnectionActive();
		}
	}

	return false;
}

std::vector<uint64_t> ConnectionManager::GetMostWorkPeers() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	std::vector<uint64_t> mostWorkPeers;

	ConnectionPtr pMostWorkPeer = GetMostWorkPeer();
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t totalDifficulty = pMostWorkPeer->GetTotalDifficulty();
		for (ConnectionPtr pConnection : m_connections)
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
	for (ConnectionPtr pConnection : m_connections)
	{
		connectedPeers.push_back(pConnection->GetConnectedPeer());
	}

	return connectedPeers;
}

std::optional<std::pair<uint64_t, ConnectedPeer>> ConnectionManager::GetConnectedPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	for (ConnectionPtr pConnection : m_connections)
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

	ConnectionPtr pConnection = GetMostWorkPeer();
	if (pConnection != nullptr)
	{
		return pConnection->GetTotalDifficulty();
	}

	return 0;
}

uint64_t ConnectionManager::GetHighestHeight() const
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	ConnectionPtr pConnection = GetMostWorkPeer();
	if (pConnection != nullptr)
	{
		return pConnection->GetHeight();
	}

	return 0;
}

uint64_t ConnectionManager::SendMessageToMostWorkPeer(const IMessage& message)
{
	std::shared_lock<std::shared_mutex> readLock(m_connectionsMutex);

	ConnectionPtr pConnection = GetMostWorkPeer();
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

	ConnectionPtr pConnection = GetConnectionById(connectionId);
	if (pConnection != nullptr)
	{
		pConnection->Send(message);
		return true;
	}

	return false;
}

void ConnectionManager::BroadcastMessage(const IMessage& message, const uint64_t sourceId)
{
	m_sendQueue.push_back(MessageToBroadcast(sourceId, message.Clone()));
}

void ConnectionManager::AddConnection(ConnectionPtr pConnection)
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
		ConnectionPtr pConnection = m_connections[i];
		const uint64_t connectionId = pConnection->GetId();

		auto iter = m_peersToBan.find(connectionId);
		if (iter == m_peersToBan.end() && pConnection->ExceedsRateLimit())
		{
			m_peersToBan.insert(std::pair<uint64_t, EBanReason>(connectionId, EBanReason::Abusive));
			iter = m_peersToBan.find(connectionId);
		}

		if (iter != m_peersToBan.end())
		{
			const EBanReason banReason = iter->second;
			LOG_WARNING_F("Banning peer (%d) at (%s) for (%s).", connectionId, pConnection->GetPeer(), BanReason::Format(banReason));

			pConnection->GetPeer().UpdateLastBanTime();
			pConnection->GetPeer().UpdateBanReason(banReason);

			m_connectionsToClose.push_back(pConnection);
			VectorUtil::Remove<ConnectionPtr>(m_connections, i);

			continue;
		}

		if (!bInactiveOnly || !pConnection->IsConnectionActive())
		{
			if (!pConnection->IsConnectionActive())
			{
				LOG_DEBUG_F("Disconnecting from inactive peer (%d) at (%s)", connectionId, pConnection->GetPeer());
			}

			m_connectionsToClose.push_back(pConnection);
			VectorUtil::Remove<ConnectionPtr>(m_connections, i);

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

	for (ConnectionPtr pConnection : m_connectionsToClose)
	{
		try
		{
			pConnection->Disconnect();
		}
		catch (std::exception& e)
		{
			LOG_ERROR("Error disconnecting: " + std::string(e.what()));
		}
	}

	m_connectionsToClose.clear();
}

void ConnectionManager::BanConnection(const uint64_t connectionId, const EBanReason banReason)
{
	std::unique_lock<std::shared_mutex> writeLock(m_connectionsMutex);

	m_peersToBan.insert(std::pair<uint64_t, EBanReason>(connectionId, banReason));
}

ConnectionPtr ConnectionManager::GetMostWorkPeer() const
{
	std::vector<ConnectionPtr> mostWorkPeers;
	uint64_t mostWork = 0;
	uint64_t mostWorkHeight = 0;
	for (ConnectionPtr pConnection : m_connections)
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

ConnectionPtr ConnectionManager::GetConnectionById(const uint64_t connectionId) const
{
	for (ConnectionPtr pConnection : m_connections)
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
		std::unique_ptr<MessageToBroadcast> pBroadcastMessage = connectionManager.m_sendQueue.copy_front();
		if (pBroadcastMessage != nullptr)
		{
			connectionManager.m_sendQueue.pop_front(1);

			// TODO: This should only broadcast to 8(?) peers. Should maybe be configurable.
			std::shared_lock<std::shared_mutex> readLock(connectionManager.m_connectionsMutex);
			for (ConnectionPtr pConnection : connectionManager.m_connections)
			{
				if (pConnection->GetId() != pBroadcastMessage->m_sourceId)
				{
					pConnection->Send(*pBroadcastMessage->m_pMessage);
				}
			}
		}
		else
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(100), connectionManager.m_terminate);
		}
	}
}