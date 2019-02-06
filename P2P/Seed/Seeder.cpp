#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"
#include "../SocketHelper.h"

#include <P2P/capabilities.h>
#include <P2P/SocketAddress.h>
#include <BlockChainServer.h>
#include <StringUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <async++.h>

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

	m_peerManager.Initialize();

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

	const int minimumConnections = seeder.m_config.GetP2PConfig().GetPreferredMinConnections();
	while (!seeder.m_terminate)
	{
		seeder.m_connectionManager.PruneConnections(true);

		const size_t numConnections = seeder.m_connectionManager.GetNumberOfActiveConnections();
		if (numConnections < minimumConnections)
		{
			const int connectionsToAdd = min(8, (minimumConnections - numConnections));
			if (connectionsToAdd == 1)
			{
				seeder.SeedNewConnection();
				continue;
			}

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

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
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

	const IPAddress localhost = IPAddress::FromIP(127, 0, 0, 1);
	const uint16_t portNumber = seeder.m_config.GetEnvironment().GetP2PPort();
	const std::optional<SOCKET> listenerSocketOpt = SocketHelper::CreateListener(localhost, portNumber);
	if (!listenerSocketOpt.has_value())
	{
		int lastErr = WSAGetLastError();
		if (lastErr != 7)
		{
			LoggerAPI::LogDebug(StringUtil::Format("FAILURE %d", lastErr));
		}

		LoggerAPI::LogError("Seeder::Thread_Listener() - Failed to create listener socket.");
		return;
	}

	const int maximumConnections = seeder.m_config.GetP2PConfig().GetMaxConnections();
	while (!seeder.m_terminate)
	{
		bool connectionAdded = false;

		if (seeder.m_connectionManager.GetNumberOfActiveConnections() < maximumConnections)
		{
			connectionAdded |= seeder.ListenForConnections(listenerSocketOpt.value());
		}

		if (!connectionAdded)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	LoggerAPI::LogTrace("Seeder::Thread_Listener() - END");
}

bool Seeder::SeedNewConnection()
{
	std::unique_ptr<Peer> pPeer = m_peerManager.GetNewPeer(Capabilities::FAST_SYNC_NODE);
	if (pPeer != nullptr)
	{
		bool connected = ConnectToPeer(*pPeer);
		if (connected)
		{
			m_peerManager.UpdatePeer(*pPeer);
		}

		return connected;
	}
	else if (!m_usedDNS.exchange(true))
	{
		std::vector<SocketAddress> peerAddresses;
		peerAddresses.emplace_back(IPAddress(EAddressFamily::IPv4, { 10, 0, 0, 7 }), m_config.GetEnvironment().GetP2PPort());
		DNSSeeder(m_config).GetPeersFromDNS(peerAddresses);

		m_peerManager.AddPeerAddresses(peerAddresses);
	}

	// TODO: Request new peers

	return false;
}

bool Seeder::ListenForConnections(const SOCKET& listenerSocket)
{
	// TODO: Don't forget to check nonces.
	const std::optional<SOCKET> socketOpt = SocketHelper::AcceptNewConnection(listenerSocket);
	if (socketOpt.has_value())
	{
		const std::optional<SocketAddress> socketAddressOpt = SocketHelper::GetSocketAddress(socketOpt.value());
		if (socketAddressOpt.has_value())
		{
			return ConnectToPeer(Peer(socketAddressOpt.value()));
		}
	}

	return false;
}

bool Seeder::ConnectToPeer(Peer& peer)
{
	Connection* pConnection = m_connectionFactory.CreateConnection(peer);
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