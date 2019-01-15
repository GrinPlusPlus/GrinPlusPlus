#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <P2P/capabilities.h>
#include <P2P/SocketAddress.h>
#include <BlockChainServer.h>
#include <StringUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>

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

	for (std::thread& thread : m_seedThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	if (m_listenerThread.joinable())
	{
		m_listenerThread.join();
	}

	m_seedThreads.clear();
	m_terminate = false;

	m_peerManager.Initialize();

	const uint8_t minConnections = (uint8_t)m_config.GetP2PConfig().GetPreferredMinConnections();
	for (uint8_t i = 0; i < minConnections; i++)
	{
		m_seedThreads.emplace_back(std::thread(Thread_Seed, std::ref(*this), i));
	}
}

void Seeder::Stop()
{
	m_terminate = true;

	if (m_listenerThread.joinable())
	{
		m_listenerThread.join();
	}

	for (std::thread& thread : m_seedThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	m_seedThreads.clear();
}

//
// Continuously checks the number of connected peers, and connects to additional peers when the number of connections drops below the minimum.
// This function operates in its own thread.
//
void Seeder::Thread_Seed(Seeder& seeder, const uint8_t threadNumber)
{
	ThreadManagerAPI::SetCurrentThreadName("SEED_THREAD" + std::to_string(threadNumber));
	LoggerAPI::LogDebug("Seeder::Thread_Seed() - BEGIN");

	const int minimumConnections = seeder.m_config.GetP2PConfig().GetPreferredMinConnections();
	while (!seeder.m_terminate)
	{
		bool connectionAdded = false;

		seeder.m_connectionManager.PruneConnections(true);

		const size_t numConnections = seeder.m_connectionManager.GetNumberOfActiveConnections();
		if (numConnections < minimumConnections)
		{
			connectionAdded |= seeder.SeedNewConnection();
		}

		// We use multiple threads for initial seeding, but only 1 to maintain connection count.
		if (connectionAdded && threadNumber > 0)
		{
			LoggerAPI::LogDebug(StringUtil::Format("Seeder::Thread_Seed() - Terminating seed thread %u since it's no longer needed.", threadNumber));
			return;
		}
		
		if (!connectionAdded)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	LoggerAPI::LogDebug("Seeder::Thread_Seed() - END");
}

//
// Continuously listens for new connections, and drops them if node already has maximum number of connections
// This function operates in its own thread.
//
void Seeder::Thread_Listener(Seeder& seeder)
{
	ThreadManagerAPI::SetCurrentThreadName("LISTENER_THREAD");
	LoggerAPI::LogDebug("Seeder::Thread_Listener() - BEGIN");

	const int maximumConnections = seeder.m_config.GetP2PConfig().GetMaxConnections();
	while (!seeder.m_terminate)
	{
		bool connectionAdded = false;

		if (seeder.m_connectionManager.GetNumberOfActiveConnections() < maximumConnections)
		{
			connectionAdded |= seeder.ListenForConnections();
		}

		if (!connectionAdded)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	LoggerAPI::LogDebug("Seeder::Thread_Listener() - END");
}

bool Seeder::SeedNewConnection()
{
	std::unique_ptr<Peer> pPeer = m_peerManager.GetNewPeer(Capabilities::FAST_SYNC_NODE);
	if (pPeer != nullptr)
	{
		Connection* pConnection = m_connectionFactory.CreateConnection(*pPeer);
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
	}
	else if (!m_usedDNS.exchange(true))
	{
		std::vector<SocketAddress> peerAddresses;
		peerAddresses.emplace_back(IPAddress(EAddressFamily::IPv4, { 10, 0, 0, 7 }), P2P::DEFAULT_PORT);
		DNSSeeder(m_config).GetPeersFromDNS(peerAddresses);

		m_peerManager.AddPeerAddresses(peerAddresses);
	}

	// TODO: Request new peers

	return false;
}

bool Seeder::ListenForConnections()
{
	// TODO: Listen for new connections. Don't forget to check nonces.

	return false;
}