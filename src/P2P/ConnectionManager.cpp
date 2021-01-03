#include "ConnectionManager.h"
#include "Sync/Syncer.h"
#include "Pipeline/Pipeline.h"
#include "Seed/Seeder.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PingMessage.h"

#include <thread>
#include <chrono>
#include <Common/Logger.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Core/Global.h>
#include <Crypto/CSPRNG.h>

ConnectionManager::ConnectionManager()
	: m_connections(std::make_shared<std::vector<ConnectionPtr>>()),
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

	ConnectionPtr pMostWorkPeer = GetMostWorkPeer(*connections);
	if (pMostWorkPeer != nullptr)
	{
		syncStatus.UpdateNetworkStatus(
			connections->size(),
			pMostWorkPeer->GetHeight(),
			pMostWorkPeer->GetTotalDifficulty()
		);
	}
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

std::vector<PeerPtr> ConnectionManager::GetMostWorkPeers() const
{
	std::vector<PeerPtr> mostWorkPeers;

	auto connections = m_connections.Read();

	ConnectionPtr pMostWorkPeer = GetMostWorkPeer(*connections);
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t totalDifficulty = pMostWorkPeer->GetTotalDifficulty();
		for (ConnectionPtr pConnection : *connections)
		{
			if (pConnection->GetTotalDifficulty() >= totalDifficulty && pConnection->GetHeight() > 0)
			{
				mostWorkPeers.push_back(pConnection->GetPeer());
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

std::optional<std::pair<uint64_t, ConnectedPeer>> ConnectionManager::GetConnectedPeer(const IPAddress& address) const
{
	auto connections = m_connections.Read();

	for (ConnectionPtr pConnection : *connections)
	{
		ConnectedPeer connectedPeer = pConnection->GetConnectedPeer();
		if (connectedPeer.GetPeer()->GetIPAddress() == address)
		{
			return std::make_optional(std::make_pair(pConnection->GetId(), std::move(connectedPeer)));
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

PeerPtr ConnectionManager::SendMessageToMostWorkPeer(const IMessage& message)
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr)
	{
		pConnection->AddToSendQueue(message);
		return pConnection->GetPeer();
	}

	return nullptr;
}

bool ConnectionManager::SendMessageToPeer(const IMessage& message, PeerConstPtr pPeer)
{
	auto connections = m_connections.Read();
	for (auto pConnection : *connections)
	{
		if (pConnection->GetIPAddress() == pPeer->GetIPAddress())
		{
			pConnection->AddToSendQueue(message);
			return true;
		}
	}

	return false;
}

void ConnectionManager::BroadcastMessage(const IMessage& message, const uint64_t sourceId)
{
	m_sendQueue.push_back(MessageToBroadcast(sourceId, message.Clone()));
}

void ConnectionManager::AddConnection(ConnectionPtr pConnection)
{
	LOG_DEBUG_F("Adding connection: {}", pConnection->GetPeer());
	m_connections.Write()->emplace_back(pConnection);
}

void ConnectionManager::PruneConnections(const bool bInactiveOnly)
{
	std::vector<ConnectionPtr> connectionsToClose;
	{
		auto connectionsWriter = m_connections.Write();
		std::vector<ConnectionPtr>& connections = *connectionsWriter;

		for (int i = (int)connections.size() - 1; i >= 0; i--)
		{
			ConnectionPtr pConnection = connections[i];
			if (!bInactiveOnly)
			{
				connectionsToClose.push_back(pConnection);
				VectorUtil::Remove<ConnectionPtr>(connections, i);
			}
			else if (!pConnection->IsConnectionActive())
			{
				LOG_DEBUG_F("Disconnecting from inactive peer ({}) at ({})", pConnection->GetId(), pConnection->GetIPAddress());
				connectionsToClose.push_back(pConnection);
				VectorUtil::Remove<ConnectionPtr>(connections, i);
			}
		}

		size_t numOutbound = 0;
		size_t numInbound = 0;
		for (auto pConnection : connections)
		{
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
	}

	for (ConnectionPtr pConnection : connectionsToClose)
	{
		try
		{
			pConnection->Disconnect(true);
		}
		catch (std::exception& e)
		{
			LOG_ERROR("Error disconnecting: " + std::string(e.what()));
		}
	}
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

	const size_t index = CSPRNG::GenerateRandom(0, mostWorkPeers.size() - 1);

	return mostWorkPeers[index];
}

void ConnectionManager::Thread_Broadcast(ConnectionManager& connectionManager)
{
	while (Global::IsRunning())
	{
		std::unique_ptr<MessageToBroadcast> pBroadcastMessage = connectionManager.m_sendQueue.copy_front();
		if (pBroadcastMessage != nullptr)
		{
			connectionManager.m_sendQueue.pop_front(1);
			LOG_DEBUG_F("Broadcasting message: {}", MessageTypes::ToString(pBroadcastMessage->m_pMessage->GetMessageType()));

			// TODO: This should only broadcast to 8(?) peers. Should maybe be configurable.
			auto pConnections = connectionManager.m_connections.Read();
			for (ConnectionPtr pConnection : *pConnections)
			{
				if (pConnection->GetId() != pBroadcastMessage->m_sourceId)
				{
					pConnection->AddToSendQueue(*pBroadcastMessage->m_pMessage);
				}
			}
		}
		else
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(100));
		}
	}
}