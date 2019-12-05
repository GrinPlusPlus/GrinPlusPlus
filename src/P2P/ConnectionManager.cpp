#include "ConnectionManager.h"
#include "Sync/Syncer.h"
#include "Pipeline/Pipeline.h"
#include "Seed/Seeder.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PingMessage.h"

#include <thread>
#include <chrono>
#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Crypto/RandomNumberGenerator.h>

ConnectionManager::ConnectionManager()
	: m_connections(std::shared_ptr<std::vector<ConnectionPtr>>(new std::vector<ConnectionPtr>())),
	m_peersToBan(std::shared_ptr<std::unordered_map<uint64_t, EBanReason>>(new std::unordered_map<uint64_t, EBanReason>())),
	m_numOutbound(0),
	m_numInbound(0)
{

}

ConnectionManager::~ConnectionManager()
{
	Shutdown();
}

void ConnectionManager::Shutdown()
{
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

std::shared_ptr<ConnectionManager> ConnectionManager::Create()
{
	auto pConnectionManager = std::shared_ptr<ConnectionManager>(new ConnectionManager());
	pConnectionManager->m_broadcastThread = std::thread(Thread_Broadcast, std::ref(*pConnectionManager));
	return pConnectionManager;
}

void ConnectionManager::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	auto connections = m_connections.Read();
	const uint64_t numActiveConnections = connections->size();
	ConnectionPtr pMostWorkPeer = GetMostWorkPeer(*connections);
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t networkHeight = pMostWorkPeer->GetHeight();
		const uint64_t networkDifficulty = pMostWorkPeer->GetTotalDifficulty();
		syncStatus.UpdateNetworkStatus(numActiveConnections, networkHeight, networkDifficulty);
	}
}

bool ConnectionManager::IsConnected(const uint64_t connectionId) const
{
	return GetConnectionById(connectionId, *m_connections.Read()) != nullptr;
}

bool ConnectionManager::IsConnected(const IPAddress& address) const
{
	auto connections = m_connections.Read();
	return std::any_of(
		connections->cbegin(),
		connections->cend(),
		[&address](ConnectionPtr pConnection)
		{
			return pConnection->GetIPAddress() == address && pConnection->IsConnectionActive();
		}
	);
}

std::vector<uint64_t> ConnectionManager::GetMostWorkPeers() const
{
	std::vector<uint64_t> mostWorkPeers;

	auto connections = m_connections.Read();

	ConnectionPtr pMostWorkPeer = GetMostWorkPeer(*connections);
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t totalDifficulty = pMostWorkPeer->GetTotalDifficulty();
		for (ConnectionPtr pConnection : *connections)
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
	auto connections = m_connections.Read();

	std::vector<ConnectedPeer> connectedPeers;
	std::transform(
		connections->cbegin(),
		connections->cend(),
		std::back_inserter(connectedPeers),
		[](ConnectionPtr pConnection) { return pConnection->GetConnectedPeer(); }
	);

	return connectedPeers;
}

std::optional<std::pair<uint64_t, ConnectedPeer>> ConnectionManager::GetConnectedPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	auto connections = m_connections.Read();

	for (ConnectionPtr pConnection : *connections)
	{
		ConnectedPeer connectedPeer = pConnection->GetConnectedPeer();
		if (connectedPeer.GetPeer().GetIPAddress() == address)
		{
			if (!portOpt.has_value() || portOpt.value() == connectedPeer.GetPeer().GetPortNumber() || !connectedPeer.GetPeer().GetIPAddress().IsLocalhost())
			{
				return std::make_optional(std::make_pair(pConnection->GetId(), std::move(connectedPeer)));
			}
		}
	}

	return std::nullopt;
}

uint64_t ConnectionManager::GetMostWork() const
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr)
	{
		return pConnection->GetTotalDifficulty();
	}

	return 0;
}

uint64_t ConnectionManager::GetHighestHeight() const
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr)
	{
		return pConnection->GetHeight();
	}

	return 0;
}

uint64_t ConnectionManager::SendMessageToMostWorkPeer(const IMessage& message)
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr)
	{
		pConnection->Send(message);
		return pConnection->GetId();
	}

	return 0;
}

bool ConnectionManager::SendMessageToPeer(const IMessage& message, const uint64_t connectionId)
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetConnectionById(connectionId, *connections);
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
	m_connections.Write()->emplace_back(pConnection);
}

void ConnectionManager::PruneConnections(const bool bInactiveOnly)
{
	std::vector<ConnectionPtr> connectionsToClose;
	{
		auto connectionsWriter = m_connections.Write();
		std::vector<ConnectionPtr>& connections = *connectionsWriter;

		auto peersToBan = m_peersToBan.Write();

		size_t numOutbound = 0;
		size_t numInbound = 0;
		for (int i = (int)connections.size() - 1; i >= 0; i--)
		{
			ConnectionPtr pConnection = connections[i];
			const uint64_t connectionId = pConnection->GetId();

			auto iter = peersToBan->find(connectionId);
			if (iter == peersToBan->end() && pConnection->ExceedsRateLimit())
			{
				peersToBan->insert(std::make_pair(connectionId, EBanReason::Abusive));
				iter = peersToBan->find(connectionId);
			}

			if (iter != peersToBan->end())
			{
				const EBanReason banReason = iter->second;
				LOG_WARNING_F("Banning peer ({}) at ({}) for ({}).", connectionId, pConnection->GetIPAddress(), BanReason::Format(banReason));

				pConnection->GetPeer().UpdateLastBanTime();
				pConnection->GetPeer().UpdateBanReason(banReason);

				connectionsToClose.push_back(pConnection);
				VectorUtil::Remove<ConnectionPtr>(connections, i);

				continue;
			}

			if (!bInactiveOnly || !pConnection->IsConnectionActive())
			{
				if (!pConnection->IsConnectionActive())
				{
					LOG_DEBUG_F("Disconnecting from inactive peer ({}) at ({})", connectionId, pConnection->GetIPAddress());
				}

				connectionsToClose.push_back(pConnection);
				VectorUtil::Remove<ConnectionPtr>(connections, i);

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
		peersToBan->clear();
	}

	for (ConnectionPtr pConnection : connectionsToClose)
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
}

void ConnectionManager::BanConnection(const uint64_t connectionId, const EBanReason banReason)
{
	m_peersToBan.Write()->insert(std::pair<uint64_t, EBanReason>(connectionId, banReason));
}

ConnectionPtr ConnectionManager::GetMostWorkPeer(const std::vector<ConnectionPtr>& connections) const
{
	std::vector<ConnectionPtr> mostWorkPeers;
	uint64_t mostWork = 0;
	uint64_t mostWorkHeight = 0;
	for (ConnectionPtr pConnection : connections)
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

	const size_t index = RandomNumberGenerator::GenerateRandom(0, mostWorkPeers.size() - 1);

	return mostWorkPeers[index];
}

ConnectionPtr ConnectionManager::GetConnectionById(const uint64_t connectionId, const std::vector<ConnectionPtr>& connections) const
{
	auto iter = std::find_if(
		connections.cbegin(),
		connections.cend(),
		[connectionId](ConnectionPtr pConnection) { return pConnection->GetId() == connectionId; }
	);

	return iter != connections.cend() ? *iter : nullptr;
}

void ConnectionManager::Thread_Broadcast(ConnectionManager& connectionManager)
{
	while (!ShutdownManagerAPI::WasShutdownRequested()) 
	{
		std::unique_ptr<MessageToBroadcast> pBroadcastMessage = connectionManager.m_sendQueue.copy_front();
		if (pBroadcastMessage != nullptr)
		{
			connectionManager.m_sendQueue.pop_front(1);

			// TODO: This should only broadcast to 8(?) peers. Should maybe be configurable.
			auto pConnections = connectionManager.m_connections.Read();
			for (ConnectionPtr pConnection : *pConnections)
			{
				if (pConnection->GetId() != pBroadcastMessage->m_sourceId)
				{
					pConnection->Send(*pBroadcastMessage->m_pMessage);
				}
			}
		}
		else
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(100), ShutdownManagerAPI::WasShutdownRequested());
		}
	}
}