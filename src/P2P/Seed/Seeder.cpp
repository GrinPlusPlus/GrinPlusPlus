#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"
#include "../ConnectionManager.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <Core/Context.h>
#include <P2P/Capabilities.h>
#include <Net/SocketAddress.h>
#include <Net/Socket.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <algorithm>

Seeder::~Seeder()
{
	LOG_INFO("Shutting down seeder");
	m_terminate = true;
	ThreadUtil::Join(m_listenerThread);
	ThreadUtil::Join(m_seedThread);
}

std::unique_ptr<Seeder> Seeder::Create(
	const std::shared_ptr<Context>& pContext,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	const IBlockChain::Ptr& pBlockChain,
	std::shared_ptr<Pipeline> pPipeline,
	SyncStatusConstPtr pSyncStatus)
{
	auto pMessageProcessor = std::make_shared<MessageProcessor>(
		pContext->GetConfig(),
		connectionManager,
		peerManager,
		pBlockChain,
		pPipeline,
		pSyncStatus
	);
	std::unique_ptr<Seeder> pSeeder(new Seeder(
		pContext,
		connectionManager,
		peerManager,
		pBlockChain,
		pMessageProcessor,
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
	LoggerAPI::SetThreadName("SEED");
	LOG_TRACE("BEGIN");

	auto lastConnectTime = std::chrono::system_clock::now() - std::chrono::seconds(10);

	const size_t minimumConnections = seeder.m_pContext->GetConfig().GetP2PConfig().GetMinConnections();
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

		ThreadUtil::SleepFor(std::chrono::milliseconds(100));
	}

	LOG_TRACE("END");
}

//
// Continuously listens for new connections, and drops them if node already has maximum number of connections
// This function operates in its own thread.
//
void Seeder::Thread_Listener(Seeder& seeder)
{
	LoggerAPI::SetThreadName("LISTENER");
	LOG_TRACE("BEGIN");

	try
	{
		const uint16_t portNumber = seeder.m_pContext->GetConfig().GetEnvironment().GetP2PPort();
		asio::ip::tcp::acceptor acceptor(*seeder.m_pAsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), portNumber));
		asio::error_code errorCode;
		acceptor.listen(asio::socket_base::max_listen_connections, errorCode);

		if (!errorCode)
		{
			const int maximumConnections = seeder.m_pContext->GetConfig().GetP2PConfig().GetMaxConnections();
			while (!seeder.m_terminate)
			{
				SocketPtr pSocket = SocketPtr(new Socket(SocketAddress(IPAddress(), portNumber)));
				// FUTURE: Always accept, but then send peers and immediately drop
				if (seeder.m_connectionManager.GetNumberOfActiveConnections() < maximumConnections)
				{
					const bool connectionAdded = pSocket->Accept(seeder.m_pAsioContext, acceptor, seeder.m_terminate);
					if (connectionAdded)
					{
						auto pPeer = seeder.m_peerManager.Write()->GetPeer(pSocket->GetIPAddress());
						Connection::CreateInbound(
							pPeer,
							pSocket,
							seeder.m_nextId++,
							seeder.m_pContext->GetConfig(),
							seeder.m_connectionManager,
							seeder.m_pMessageProcessor,
							seeder.m_pSyncStatus
						);
					}
				}
				else
				{
					ThreadUtil::SleepFor(std::chrono::milliseconds(10));
				}
			}

			acceptor.cancel();
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Listener failed with error: {}", e.what());
	}

	LOG_TRACE("END");
}

ConnectionPtr Seeder::SeedNewConnection()
{
	PeerPtr pPeer = m_peerManager.Write()->GetNewPeer(Capabilities::FAST_SYNC_NODE);
	if (pPeer != nullptr)
	{
		LOG_TRACE_F("Attempting to seed: {}", pPeer);
		return Connection::CreateOutbound(
			pPeer,
			m_nextId++,
			m_pContext->GetConfig(),
			m_connectionManager,
			m_pMessageProcessor,
			m_pSyncStatus
		);
	}
	else if (!m_usedDNS.exchange(true))
	{
		std::vector<SocketAddress> peerAddresses = DNSSeeder(m_pContext->GetConfig()).GetPeersFromDNS();

		m_peerManager.Write()->AddFreshPeers(peerAddresses);
	}

	return nullptr;
}