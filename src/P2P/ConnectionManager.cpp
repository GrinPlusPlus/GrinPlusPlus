#include "ConnectionManager.h"
#include "Sync/Syncer.h"
#include "Pipeline/Pipeline.h"
#include "Seed/Seeder.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PingMessage.h"

#include <thread>
#include <chrono>
#include <Common/Logger.h>
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
		ThreadUtil::Join(m_pingThread);

		PruneConnections(false);
	}
	catch (const std::exception& e)
	{
		LOG_ERROR(e.what());
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception thrown");
	}
}

std::shared_ptr<ConnectionManager> ConnectionManager::Create()
{
	auto pConnectionManager = std::shared_ptr<ConnectionManager>(new ConnectionManager());
	pConnectionManager->m_pingThread = std::thread(ThreadPing, std::ref(*pConnectionManager));
	return pConnectionManager;
}

void ConnectionManager::UpdateSyncStatus(SyncStatus& syncStatus) const
{
	auto connections = m_connections.Read();

	ConnectionPtr pMostWorkPeer = GetMostWorkPeer(*connections);
	syncStatus.UpdateNetworkStatus(
		connections->size(),
		pMostWorkPeer != nullptr ? pMostWorkPeer->GetHeight() : NULL,
		pMostWorkPeer != nullptr ? pMostWorkPeer->GetTotalDifficulty() : NULL
	);
}

bool ConnectionManager::IsConnected(const IPAddress& address) const
{
	auto connections = m_connections.Read();
	return std::any_of(
		connections->cbegin(),
		connections->cend(),
		[&address](const ConnectionPtr& pConnection)
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
	if (pMostWorkPeer != nullptr) {
		const uint64_t totalDifficulty = pMostWorkPeer->GetTotalDifficulty();
		for (const ConnectionPtr& pConnection : *connections) {
			if (pConnection->GetTotalDifficulty() >= totalDifficulty && pConnection->GetHeight() > 0) {
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
		[](const ConnectionPtr& pConnection) { return pConnection->GetConnectedPeer(); }
	);

	return connectedPeers;
}

uint64_t ConnectionManager::GetMostWork() const
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr) {
		return pConnection->GetTotalDifficulty();
	}

	return 0;
}

uint64_t ConnectionManager::GetHighestHeight() const
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr) {
		return pConnection->GetHeight();
	}

	return 0;
}

PeerPtr ConnectionManager::SendMessageToMostWorkPeer(const IMessage& message)
{
	auto connections = m_connections.Read();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr) {
		pConnection->SendAsync(message);
		return pConnection->GetPeer();
	}

	return nullptr;
}

bool ConnectionManager::SendMessageToPeer(const IMessage& message, PeerConstPtr pPeer)
{
	auto connections = *m_connections.Read().GetShared();
	for (auto pConnection : connections) {
		if (pConnection->GetIPAddress() == pPeer->GetIPAddress()) {
			pConnection->SendAsync(message);
			return true;
		}
	}

	return false;
}

void ConnectionManager::BroadcastMessage(const IMessage& message, const uint64_t sourceId)
{
	LOG_DEBUG("Broadcasting message: {}", MessageTypes::ToString(message.GetMessageType()));

	// TODO: This should only broadcast to 8(?) peers. Should maybe be configurable.
	auto connections = *m_connections.Read().GetShared();
	for (ConnectionPtr pConnection : connections) {
		if (pConnection->GetId() != sourceId) {
			pConnection->SendAsync(message);
		}
	}
}

void ConnectionManager::AddConnection(ConnectionPtr pConnection)
{
	LOG_DEBUG("Adding connection: {}", pConnection->GetPeer());
	m_connections.Write()->emplace_back(pConnection);
}

ConnectionPtr ConnectionManager::GetConnection(const uint64_t connectionId) const
{
	auto connections = m_connections.Read();
	for (auto pConnection : *connections) {
		if (pConnection->GetId() == connectionId) {
			return pConnection;
		}
	}

	return nullptr;
}

void ConnectionManager::PruneConnections(const bool bInactiveOnly)
{
	auto connectionsWriter = m_connections.Write();
	std::vector<ConnectionPtr>& connections = *connectionsWriter;

	for (auto iter = connections.begin(); iter < connections.end(); iter++) {
		ConnectionPtr pConnection = *iter;
		if (!bInactiveOnly || !pConnection->IsConnectionActive()) {
			try { 
				pConnection->Disconnect();
			}
			catch (std::exception& e) {
				LOG_ERROR("Error disconnecting: {}", e.what());
			}
			connections.erase(iter);
		}
	}

	m_numInbound = std::accumulate(
		connections.begin(), connections.end(), (size_t)0,
		[](size_t inbound, const ConnectionPtr& pConnection) {
			return pConnection->GetDirection() == EDirection::INBOUND ? inbound + 1 : inbound;
		}
	);

	m_numOutbound = connections.size() - m_numInbound;
}

ConnectionPtr ConnectionManager::GetMostWorkPeer(const std::vector<ConnectionPtr>& connections) const
{
	std::vector<ConnectionPtr> mostWorkPeers;
	uint64_t mostWork = 0;
	uint64_t mostWorkHeight = 0;
	for (const ConnectionPtr& pConnection : connections)
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

void ConnectionManager::ThreadPing(ConnectionManager& connectionManager)
{
	while (Global::IsRunning())
	{
		auto connections = *connectionManager.m_connections.Read().GetShared();
		for (const ConnectionPtr& pConnection : connections) {
			pConnection->CheckPing();
		}

		ThreadUtil::SleepFor(std::chrono::seconds(2));
	}
}