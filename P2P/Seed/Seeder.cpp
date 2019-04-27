#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <P2P/capabilities.h>
#include <Net/SocketAddress.h>
#include <BlockChain/BlockChainServer.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <async++.h>
#include <algorithm>

Seeder::Seeder(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer)
	: m_config(config), 
	m_connectionManager(connectionManager), 
	m_peerManager(peerManager), 
	m_connectionFactory(config, connectionManager, peerManager, blockChainServer)
{

}

void Seeder::Start()
{
	m_terminate = true;

	if (m_seedThread.joinable())
	{
		m_seedThread.join();
	}

	if (m_listenerThread.joinable())
	{
		m_listenerThread.join();
	}

	m_terminate = false;

	m_seedThread = std::thread(Thread_Seed, std::ref(*this));
	m_listenerThread = std::thread(Thread_Listener, std::ref(*this));
}

void Seeder::Stop()
{
	m_terminate = true;

	if (m_listenerThread.joinable())
	{
		m_listenerThread.join();
	}

	if (m_seedThread.joinable())
	{
		m_seedThread.join();
	}
}

//
// Continuously checks the number of connected peers, and connects to additional peers when the number of connections drops below the minimum.
// This function operates in its own thread.
//
void Seeder::Thread_Seed(Seeder& seeder)
{
	ThreadManagerAPI::SetCurrentThreadName("SEED_THREAD");
	LoggerAPI::LogTrace("Seeder::Thread_Seed() - BEGIN");

	std::chrono::system_clock::time_point lastPruneTime = std::chrono::system_clock::now();

	const size_t minimumConnections = seeder.m_config.GetP2PConfig().GetPreferredMinConnections();
	while (!seeder.m_terminate)
	{
		auto now = std::chrono::system_clock::now();
		if (lastPruneTime + std::chrono::milliseconds(250) < now)
		{
			seeder.m_connectionManager.PruneConnections(true);
			lastPruneTime = now;
		}

		const size_t numConnections = seeder.m_connectionManager.GetNumberOfActiveConnections();
		if (numConnections < minimumConnections)
		{
			const size_t connectionsToAdd = std::min((size_t)5, (minimumConnections - numConnections));
			if (connectionsToAdd == 1)
			{
				seeder.SeedNewConnection();
			}
			else
			{
				// Using when_any to find task which finishes first
				std::vector<async::task<void>> tasks;
				for (size_t i = 0; i < connectionsToAdd; i++)
				{
					tasks.push_back(async::spawn([&seeder] { seeder.SeedNewConnection(); }));
				}

				for (auto& task : tasks)
				{
					task.wait();
				}
			}

			ThreadUtil::SleepFor(std::chrono::milliseconds(2), seeder.m_terminate);
		}
		else
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(25), seeder.m_terminate);
		}
	}

	LoggerAPI::LogTrace("Seeder::Thread_Seed() - END");
}

//
// Continuously listens for new connections, and drops them if node already has maximum number of connections
// This function operates in its own thread.
//
void Seeder::Thread_Listener(Seeder& seeder)
{
	ThreadManagerAPI::SetCurrentThreadName("LISTENER_THREAD");
	LoggerAPI::LogTrace("Seeder::Thread_Listener() - BEGIN");

	const IPAddress listenerAddress = IPAddress::FromIP(0, 0, 0, 0);
	const uint16_t portNumber = seeder.m_config.GetEnvironment().GetP2PPort();
	const std::optional<Listener> listenerSocketOpt = Listener::Create(listenerAddress, portNumber);
	if (!listenerSocketOpt.has_value())
	{
		LoggerAPI::LogError("Seeder::Thread_Listener() - Failed to create listener socket.");
		return;
	}

	const int maximumConnections = seeder.m_config.GetP2PConfig().GetMaxConnections();
	while (!seeder.m_terminate)
	{
		bool connectionAdded = false;

		// TODO: Always accept, but then send peers and immediately drop
		if (seeder.m_connectionManager.GetNumberOfActiveConnections() < maximumConnections)
		{
			connectionAdded |= seeder.ListenForConnections(listenerSocketOpt.value());
		}

		if (!connectionAdded)
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(10), seeder.m_terminate);
		}
	}

	LoggerAPI::LogTrace("Seeder::Thread_Listener() - END");
}

bool Seeder::SeedNewConnection()
{
	std::unique_ptr<Peer> pPeer = m_peerManager.GetNewPeer(Capabilities::FAST_SYNC_NODE);
	if (pPeer != nullptr)
	{
		return ConnectToPeer(*pPeer, EDirection::OUTBOUND, std::nullopt);
	}
	else if (!m_usedDNS.exchange(true))
	{
		std::vector<SocketAddress> peerAddresses = DNSSeeder(m_config).GetPeersFromDNS();

		m_peerManager.AddFreshPeers(peerAddresses);
	}

	// TODO: Request new peers

	return false;
}

bool Seeder::ListenForConnections(const Listener& listenerSocket)
{
	const std::optional<Socket> socketOpt = listenerSocket.AcceptNewConnection();
	if (socketOpt.has_value())
	{
		return ConnectToPeer(Peer(socketOpt.value().GetSocketAddress()), EDirection::INBOUND, socketOpt);
	}

	return false;
}

bool Seeder::ConnectToPeer(Peer& peer, const EDirection& direction, const std::optional<Socket>& socketOpt)
{
	Connection* pConnection = m_connectionFactory.CreateConnection(peer, direction, socketOpt);
	if (pConnection != nullptr)
	{
		if (m_peerManager.ArePeersNeeded(Capabilities::ECapability::FAST_SYNC_NODE))
		{
			const Capabilities capabilites(Capabilities::ECapability::FAST_SYNC_NODE);
			const GetPeerAddressesMessage getPeerAddressesMessage(capabilites);
			pConnection->Send(getPeerAddressesMessage);
		}

		m_connectionManager.AddConnection(pConnection);
		return true;
	}

	return false;
}