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
#include <functional>
#include <algorithm>

Seeder::~Seeder()
{
    LOG_INFO("Shutting down seeder");
    m_terminate = true;

    m_pAsioContext->stop();

    std::unique_lock<std::mutex> lock(m_mutex);
    ThreadUtil::Join(m_seedThread);
    ThreadUtil::Join(m_asioThread);
}

std::unique_ptr<Seeder> Seeder::Create(
    ConnectionManager& connectionManager,
    Locked<PeerManager> peerManager,
    const IBlockChain::Ptr& pBlockChain,
    std::shared_ptr<Pipeline> pPipeline,
    SyncStatusConstPtr pSyncStatus)
{
    auto pMessageProcessor = std::make_shared<MessageProcessor>(
        connectionManager,
        peerManager,
        pBlockChain,
        pPipeline,
        pSyncStatus
    );
    std::unique_ptr<Seeder> pSeeder(new Seeder(
        connectionManager,
        peerManager,
        pBlockChain,
        pMessageProcessor,
        pSyncStatus
    ));

    pSeeder->m_asioThread = std::thread(Thread_AsioContext, std::ref(*pSeeder.get()));
    pSeeder->m_seedThread = std::thread(Thread_Seed, std::ref(*pSeeder.get()));

    return pSeeder;
}

void Seeder::Thread_AsioContext(Seeder& seeder)
{
    LoggerAPI::SetThreadName("ASIO");

    seeder.StartListener();

    while (!seeder.m_terminate) {
        asio::error_code ec;
        seeder.m_pAsioContext->run(ec);
    }
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

    const size_t minimumConnections = Global::GetConfig().GetMinPeers();
    while (!seeder.m_terminate && Global::IsRunning()) {
        try {
            seeder.m_connectionManager.PruneConnections(true);

            auto now = std::chrono::system_clock::now();
            const size_t numOutbound = seeder.m_connectionManager.GetNumOutbound();
            if (numOutbound < minimumConnections && lastConnectTime + std::chrono::seconds(5) < now) {
                lastConnectTime = now;

                const size_t connectionsToAdd = (std::min)((size_t)15, (minimumConnections - numOutbound));
                for (size_t i = 0; i < connectionsToAdd; i++) {
                    seeder.SeedNewConnection();
                }
            }
        }
        catch (std::exception& e) {
            LOG_WARNING_F("Exception thrown: {}", e.what());
        }

        ThreadUtil::SleepFor(std::chrono::milliseconds(100));
    }

    LOG_TRACE("END");
}

void Seeder::StartListener()
{
    try {
        m_pAcceptor = std::make_shared<asio::ip::tcp::acceptor>(
            *m_pAsioContext,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), Global::GetConfig().GetP2PPort())
        );

        m_pSocket = std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext);
        m_pAcceptor->async_accept(*m_pSocket, std::bind(&Seeder::Accept, this, std::placeholders::_1));
    }
    catch (std::exception& e) {
        LOG_ERROR_F("Listener failed with error: {}", e.what());
    }
}

void Seeder::Accept(const asio::error_code& ec)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!ec) {
        if (m_connectionManager.GetNumberOfActiveConnections() < Global::GetConfig().GetMaxPeers()) {
            SocketAddress socket_address = SocketAddress::FromEndpoint(m_pSocket->remote_endpoint());
            SocketPtr pSocket(new Socket(SocketAddress::FromEndpoint(m_pSocket->remote_endpoint()), m_pAsioContext, m_pSocket));
            pSocket->SetOpen(true);

            auto pPeer = m_peerManager.Write()->GetPeer(pSocket->GetIPAddress());
            if (!pPeer->IsBanned()) {
                auto pConnection = std::make_shared<Connection>(
                    pSocket,
                    m_nextId++,
                    m_connectionManager,
                    ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
                    m_pSyncStatus,
                    m_pMessageProcessor
                );
                pConnection->Connect();
            }
        } else {
            asio::error_code ignoreError;
            m_pSocket->close(ignoreError);
        }

        m_pSocket = std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext);
        m_pAcceptor->async_accept(*m_pSocket, std::bind(&Seeder::Accept, this, std::placeholders::_1));
    }
}

void Seeder::SeedNewConnection()
{
    PeerPtr pPeer = m_peerManager.Write()->GetNewPeer(Capabilities::FAST_SYNC_NODE);
    if (pPeer != nullptr) {
        LOG_TRACE_F("Attempting to seed: {}", pPeer);

        ConnectedPeer connectedPeer(
            pPeer,
            EDirection::OUTBOUND,
            Global::GetConfig().GetP2PPort()
        );
        SocketPtr pSocket(new Socket(
            connectedPeer.GetSocketAddress(),
            m_pAsioContext,
            std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext)
        ));

        std::cout << "Attempt to connect to " << connectedPeer.Format() << std::endl;
        ConnectionPtr pConnection = std::make_shared<Connection>(
            pSocket,
            m_nextId++,
            m_connectionManager,
            connectedPeer,
            m_pSyncStatus,
            m_pMessageProcessor
        );
        pConnection->Connect();
    } else if (!m_usedDNS.exchange(true)) {
        std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();

        m_peerManager.Write()->AddFreshPeers(peerAddresses);
    }
}