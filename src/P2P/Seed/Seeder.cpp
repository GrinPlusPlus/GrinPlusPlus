#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <P2P/Capabilities.h>
#include <Net/SocketAddress.h>
#include <Net/Socket.h>
#include <BlockChain/BlockChainServer.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <algorithm>

Seeder::Seeder(
	const Config& config,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	IBlockChainServerPtr pBlockChainServer,
	std::shared_ptr<Pipeline> pPipeline,
	SyncStatusConstPtr pSyncStatus)
	: m_config(config),
	m_connectionManager(connectionManager),
	m_peerManager(peerManager),
	m_pBlockChainServer(pBlockChainServer),
	m_pPipeline(pPipeline),
	m_pSyncStatus(pSyncStatus),
	m_pContext(std::make_shared<asio::io_context>()),
	m_terminate(false)
{

}

Seeder::~Seeder()
{
	m_terminate = true;
	ThreadUtil::Join(m_listenerThread);
	ThreadUtil::Join(m_seedThread);
}

std::shared_ptr<Seeder> Seeder::Create(
	const Config& config,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	IBlockChainServerPtr pBlockChainServer,
	std::shared_ptr<Pipeline> pPipeline,
	SyncStatusConstPtr pSyncStatus)
{
	std::shared_ptr<Seeder> pSeeder = std::shared_ptr<Seeder>(new Seeder(
		config,
		connectionManager,
		peerManager,
		pBlockChainServer,
		pPipeline,
		pSyncStatus
	));
	pSeeder->m_seedThread = std::thread(Thread_Seed, std::ref(*pSeeder.get()));
	pSeeder->m_listenerThread = std::thread(Thread_Listener, std::ref(*pSeeder.get()));
	return pSeeder;
}

//
// Continuously checks the number of connected peers, and connects to additional peers when the number of connections drops below the minimum.
// This function operates in its own thread.
//
void Seeder::Thread_Seed(Seeder& seeder)
{
	ThreadManagerAPI::SetCurrentThreadName("SEED");
	LOG_TRACE("BEGIN");

	std::chrono::system_clock::time_point lastConnectTime = std::chrono::system_clock::now() - std::chrono::seconds(10);

	const size_t minimumConnections = seeder.m_config.GetNodeConfig().GetP2P().GetPreferredMinConnections();
	while (!seeder.m_terminate)
	{
		try
		{
			seeder.m_connectionManager.PruneConnections(true);

			auto now = std::chrono::system_clock::now();
			const size_t numOutbound = seeder.m_connectionManager.GetNumOutbound();
			if (numOutbound < minimumConnections && lastConnectTime + std::chrono::seconds(2) < now)
			{
				lastConnectTime = now;
				const size_t connectionsToAdd = (std::min)((size_t)15, (minimumConnections - numOutbound));
				LOG_TRACE_F("Attempting to add {} connections", connectionsToAdd);
				for (size_t i = 0; i < connectionsToAdd; i++)
				{
					seeder.SeedNewConnection();
				}
			}
		}
		catch (std::exception& e)
		{
			LOG_WARNING_F("Exception thrown: {}", e.what());
		}

		ThreadUtil::SleepFor(std::chrono::milliseconds(100), seeder.m_terminate);
	}

	LOG_TRACE("END");
}

//
// Continuously listens for new connections, and drops them if node already has maximum number of connections
// This function operates in its own thread.
//
void Seeder::Thread_Listener(Seeder& seeder)
{
	ThreadManagerAPI::SetCurrentThreadName("LISTENER");
	LOG_TRACE("BEGIN");

	const uint16_t portNumber = seeder.m_config.GetEnvironment().GetP2PPort();
	asio::ip::tcp::acceptor acceptor(*seeder.m_pContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), portNumber));
	asio::error_code errorCode;
	acceptor.listen(asio::socket_base::max_listen_connections, errorCode);

	if (!errorCode)
	{
		const int maximumConnections = seeder.m_config.GetNodeConfig().GetP2P().GetMaxConnections();
		while (!seeder.m_terminate)
		{
			SocketPtr pSocket = SocketPtr(new Socket(SocketAddress(IPAddress(), portNumber)));
			// FUTURE: Always accept, but then send peers and immediately drop
			if (seeder.m_connectionManager.GetNumberOfActiveConnections() < maximumConnections)
			{
				const bool connectionAdded = pSocket->Accept(seeder.m_pContext, acceptor, seeder.m_terminate);
				if (connectionAdded)
				{
					ConnectionPtr pConnection = Connection::Create(
						pSocket,
						seeder.m_nextId++,
						seeder.m_config,
						seeder.m_connectionManager,
						seeder.m_peerManager,
						seeder.m_pBlockChainServer,
						ConnectedPeer(seeder.m_peerManager.Write()->GetPeer(pSocket->GetIPAddress()), EDirection::INBOUND, pSocket->GetPort()),
						seeder.m_pPipeline,
						seeder.m_pSyncStatus
					);
				}
			}
			else
			{
				ThreadUtil::SleepFor(std::chrono::milliseconds(10), seeder.m_terminate);
			}
		}

		acceptor.cancel();
	}

	LOG_TRACE("END");
}

ConnectionPtr Seeder::SeedNewConnection()
{
	PeerPtr pPeer = m_peerManager.Write()->GetNewPeer(Capabilities::FAST_SYNC_NODE);
	if (pPeer != nullptr)
	{
		LOG_TRACE_F("Trying to seed: {}", pPeer);
		ConnectedPeer connectedPeer(pPeer, EDirection::OUTBOUND, m_config.GetEnvironment().GetP2PPort());
		SocketAddress address(pPeer->GetIPAddress(), m_config.GetEnvironment().GetP2PPort());
		SocketPtr pSocket = SocketPtr(new Socket(std::move(address)));
		ConnectionPtr pConnection = Connection::Create(
			pSocket,
			m_nextId++,
			m_config,
			m_connectionManager,
			m_peerManager,
			m_pBlockChainServer,
			connectedPeer,
			m_pPipeline,
			m_pSyncStatus
		);

		return pConnection;
	}
	else if (!m_usedDNS.exchange(true))
	{
		std::vector<SocketAddress> peerAddresses = DNSSeeder(m_config).GetPeersFromDNS();

		m_peerManager.Write()->AddFreshPeers(peerAddresses);
	}
	else
	{
		m_connectionManager.BroadcastMessage(GetPeerAddressesMessage(Capabilities::FAST_SYNC_NODE), 0);
	}

	return nullptr;
}