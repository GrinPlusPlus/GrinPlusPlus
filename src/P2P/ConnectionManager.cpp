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
	pConnectionManager->m_pingThread = std::thread(ThreadPing, std::ref(*pConnectionManager));
	return pConnectionManager;
}

void ConnectionManager::UpdateSyncStatus(SyncStatus& syncStatus) 
{
	ConnectionPtr pMostWorkPeer = nullptr;
	auto mostWorkPeers = GetMostWorkPeers();
	
	if (mostWorkPeers.size() > 0)
	{
		const size_t index = CSPRNG::GenerateRandom(0, mostWorkPeers.size() - 1);
		pMostWorkPeer = GetConnection(mostWorkPeers[index]->GetIPAddress());
	}

	syncStatus.UpdateNetworkStatus(
		mostWorkPeers.size(),
		pMostWorkPeer != nullptr ? pMostWorkPeer->GetHeight() : NULL,
		pMostWorkPeer != nullptr ? pMostWorkPeer->GetTotalDifficulty() : NULL
	);

	UpdateConnectionsNumber();
}

void ConnectionManager::UpdateConnectionsNumber() 
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();

	m_numInbound = std::accumulate(
		connections->begin(), connections->end(), (size_t)0,
		[](size_t inbound, const ConnectionPtr& pConnection) {
			return pConnection->GetDirection() == EDirection::INBOUND ? inbound + 1 : inbound;
		}
	);

	m_numOutbound.exchange(connections->size() - m_numInbound, std::memory_order_release);
}

bool ConnectionManager::IsConnected(const IPAddress& address) const
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	return std::any_of(
		connections->cbegin(),
		connections->cend(),
		[&address](const ConnectionPtr& pConnection)
		{
			return pConnection->GetIPAddress() == address && pConnection->IsConnectionActive();
		}
	);
}

std::vector<PeerPtr> ConnectionManager::GetMostWorkPeers()
{
	std::vector<PeerPtr> mostWorkPeers;
	
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();

	ConnectionPtr pMostWorkPeer = GetMostWorkPeer(*connections);
	if (pMostWorkPeer != nullptr)
	{
		const uint64_t totalDifficulty = pMostWorkPeer->GetTotalDifficulty();
		for (const ConnectionPtr& pConnection : *connections) 
		{
			try 
			{
				if (pConnection->GetTotalDifficulty() >= totalDifficulty &&
					pConnection->GetHeight() > 0)
				{
					mostWorkPeers.push_back(pConnection->GetPeer());
				}
			} catch (...) { }
		}
	}

	return mostWorkPeers;
}

std::vector<ConnectedPeer> ConnectionManager::GetConnectedPeers()
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();

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
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr) {
		return pConnection->GetTotalDifficulty();
	}

	return 0;
}

uint64_t ConnectionManager::GetHighestHeight() const
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection != nullptr) {
		return pConnection->GetHeight();
	}

	return 0;
}

PeerPtr ConnectionManager::SendMessageToMostWorkPeer(const IMessage& message)
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);

	if (pConnection != nullptr) {
		if (pConnection->IsBusy())
		{
			return nullptr;
		}
		LOG_DEBUG_F("Sending message: {} to connection: {}", MessageTypes::ToString(message.GetMessageType()), pConnection->GetPeer());
		pConnection->IsBusy(true);
		auto sent = pConnection->SendSync(message);
		pConnection->IsBusy(false);
		if(sent)
		{
			return pConnection->GetPeer();
		}
	}

	return nullptr;
}


std::pair <PeerPtr, std::unique_ptr<RawMessage>> ConnectionManager::ExchangeMessageWithMostWorkPeer(const IMessage& message)
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	ConnectionPtr pConnection = GetMostWorkPeer(*connections);
	if (pConnection == nullptr || pConnection->GetPeer()->GetBanReason() == EBanReason::FraudHeight)
	{
		return std::make_pair(nullptr, nullptr);
	}

	try
	{
		if (pConnection->GetSocket()->HasReceivedData() && pConnection->ReceiveProcessSync())
		{
			LOG_TRACE_F("Socket read before sending message to: {}", pConnection->GetPeer());
		}
	}
	catch (...) {}

	LOG_TRACE_F("Sending message: {} to connection: {}", MessageTypes::ToString(message.GetMessageType()), pConnection->GetPeer());
	return std::make_pair(pConnection->GetPeer(), pConnection->SendReceiveSync(message));
}

bool ConnectionManager::SendMessageToPeer(const IMessage& message, PeerConstPtr pPeer)
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections = * m_connections.Read().GetShared();
	for (auto pConnection : connections) {
		if (pConnection->GetIPAddress() == pPeer->GetIPAddress() && !pConnection->IsBusy()) {
			LOG_DEBUG_F("Sending message: {} to connection: {}", MessageTypes::ToString(message.GetMessageType()), pConnection->GetPeer());
			return pConnection->SendSync(message);
		}
	}

	return false;
}

bool ConnectionManager::ExchangeMessageWithPeer(const IMessage& message, PeerConstPtr pPeer)
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections = * m_connections.Read().GetShared();
	for (auto pConnection : connections)
	{
		if (pConnection->GetIPAddress() == pPeer->GetIPAddress())
		{
			if (pConnection->IsBusy()) 
			{
				return false;
			}

			try
			{
				if (pConnection->GetSocket()->HasReceivedData() && pConnection->ReceiveProcessSync())
				{
					LOG_TRACE_F("Socket read before sending message to: {}", pConnection->GetPeer());
				}
			}
			catch (...) {}

			LOG_TRACE_F("Sending message: {} to connection: {}", MessageTypes::ToString(message.GetMessageType()), pConnection->GetPeer());
			return pConnection->SendReceiveSync(message) != nullptr;
		}
	}

	return false;
}

void ConnectionManager::BroadcastMessage(const IMessage& message, const uint64_t sourceId)
{
	LOG_DEBUG_F("Broadcasting message: {}", MessageTypes::ToString(message.GetMessageType()));
	std::unique_lock<std::mutex> write_lock(m_mutex);

	// TODO: This should only broadcast to 8(?) peers. Should maybe be configurable.
	auto connections = * m_connections.Read().GetShared();
	for (ConnectionPtr pConnection : connections) {
		if (pConnection->GetId() != sourceId && !pConnection->IsBusy()) {
			pConnection->SendSync(message);
		}
	}
}

void ConnectionManager::AddConnection(ConnectionPtr pConnection)
{
	LOG_DEBUG_F("Adding connection: {}", pConnection->GetPeer());
	std::unique_lock<std::mutex> write_lock(m_mutex);

	m_connections.Write().GetShared()->emplace_back(pConnection);
}


ConnectionPtr ConnectionManager::GetConnection(const uint64_t connectionId) const
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	for (auto pConnection : *connections) {
		if (pConnection->GetId() == connectionId) {
			return pConnection;
		}
	}

	return nullptr;
}

ConnectionPtr ConnectionManager::GetConnection(const IPAddress& ipAddress) const
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	for (auto pConnection : *connections) {
		if (pConnection->GetIPAddress() == ipAddress) {
			return pConnection;
		}
	}
	return nullptr;
}

bool ConnectionManager::ConnectionExist(const IPAddress& ipAddress) const
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connections =  m_connections.Read().GetShared();
	return std::any_of(
		connections->cbegin(),
		connections->cend(),
		[&ipAddress](const ConnectionPtr& pConnection)
		{
			return pConnection->GetIPAddress() == ipAddress;
		}
	);
}

void ConnectionManager::PruneConnections(const bool bInactiveOnly)
{
	std::unique_lock<std::mutex> write_lock(m_mutex);

	auto connectionsWriter = m_connections.Write().GetShared();
	std::vector<ConnectionPtr>& connections = *connectionsWriter;

	for (auto iter = connections.begin(); iter < connections.end(); iter++) {
		ConnectionPtr pConnection = *iter;
		if (bInactiveOnly)
		{
			try {
				if (pConnection->GetHeight() == 0 || !pConnection->IsConnectionActive())
				{
					LOG_DEBUG_F("Disconnecting from: {}", pConnection);
					pConnection->Disconnect();
				}
			}
			catch (std::exception& e) 
			{
				LOG_ERROR("Error disconnecting: " + std::string(e.what()));
			}
			connections.erase(iter);
		}
		else
		{
			pConnection->Disconnect();
		}
	}	
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
	do
	{
		auto connections = *connectionManager. m_connections.Read().GetShared();
		for (const ConnectionPtr& pConnection : connections)
		{
			if (!pConnection->IsBusy())
			{
				pConnection->PingSync();
			}
		}
		ThreadUtil::SleepFor(std::chrono::milliseconds(500));
	} while (Global::IsRunning());
}