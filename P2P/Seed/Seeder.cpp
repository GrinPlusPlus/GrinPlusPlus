#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <P2P/capabilities.h>
#include <P2P/SocketAddress.h>
#include <BlockChainServer.h>
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
	m_terminate = false;

	if (m_seedThread.joinable())
	{
		m_seedThread.join();
	}

	m_seedThread = std::thread(Thread_Seed, std::ref(*this));
}

void Seeder::Stop()
{
	m_terminate = true;

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

	LoggerAPI::LogInfo("Seeder::Thread_Seed() - BEGIN");
	seeder.m_peerManager.Initialize();

	const int minimumConnections = seeder.m_config.GetP2PConfig().GetPreferredMinConnections();
	const int maximumConnections = seeder.m_config.GetP2PConfig().GetMaxConnections();

	while (!seeder.m_terminate)
	{
		bool connectionAdded = false;

		seeder.m_connectionManager.PruneConnections(true);

		const size_t numConnections = seeder.m_connectionManager.GetNumberOfActiveConnections();
		if (numConnections < minimumConnections)
		{
			connectionAdded |= seeder.SeedNewConnection();
		}

		if (seeder.m_connectionManager.GetNumberOfActiveConnections() < maximumConnections)
		{
			connectionAdded |= seeder.ListenForConnections();
		}
		
		if (!connectionAdded)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	LoggerAPI::LogInfo("Seeder::Thread_Seed() - END");
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
	else if (!m_usedDNS)
	{
		std::vector<SocketAddress> peerAddresses;
		peerAddresses.emplace_back(IPAddress(EAddressFamily::IPv4, { 10, 0, 0, 7 }), P2P::DEFAULT_PORT);
		DNSSeeder().GetPeersFromDNS(peerAddresses);
		m_usedDNS = true;

		m_peerManager.AddPeerAddresses(peerAddresses);
	}

	return false;
}

bool Seeder::ListenForConnections()
{
	// TODO: Listen for new connections. Don't forget to check nonces.
	return false;
}