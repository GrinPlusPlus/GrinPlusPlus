#include "Seeder.h"
#include "DNSSeeder.h"
#include "PeerManager.h"

#include "../ConnectionManager.h"
#include "../Connection.h"

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
    m_pAcceptor->cancel();
    
    std::unique_lock<std::mutex> lock(m_mutex);

    ThreadUtil::Join(m_seedThread);
    ThreadUtil::Join(m_listenerThread);
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

    pSeeder->m_pServerSocket = std::make_shared<asio::ip::tcp::socket>(*pSeeder->m_pAsioContext);

    pSeeder->m_listenerThread = std::thread(Thread_Listener, std::ref(*pSeeder.get()));
    pSeeder->m_seedThread = std::thread(Thread_Seed, std::ref(*pSeeder.get()));

    return pSeeder;
}

void Seeder::Thread_Listener(Seeder& seeder)
{
    LoggerAPI::SetThreadName("LISTENER");
    LOG_DEBUG("BEGIN");

    asio::error_code errorCode;

    try
    {
        seeder.m_pAcceptor = std::make_shared<asio::ip::tcp::acceptor>(*seeder.m_pAsioContext);
        auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), Global::GetConfig().GetP2PPort());

        seeder.m_pAcceptor->open(endpoint.protocol());
        seeder.m_pAcceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
        seeder.m_pAcceptor->bind(endpoint);
        seeder.m_pAcceptor->listen(asio::socket_base::max_listen_connections, errorCode);

        if (errorCode)
        {
            throw new std::runtime_error(errorCode.message());
        }

        do
        {
            SocketPtr pSocket(
                new Socket(
                    SocketAddress::FromEndpoint(endpoint),
                    seeder.m_pAsioContext,
                    std::make_shared<asio::ip::tcp::socket>(*seeder.m_pAsioContext)
                )
            );

            try
            {
                seeder.m_pAcceptor->accept(*pSocket->GetAsioSocket());
                
                auto ipAddress = IPAddress(pSocket->GetAsioSocket()->remote_endpoint().address());

                if (seeder.m_connectionManager.GetNumberOfActiveConnections() == Global::GetConfig().GetMaxPeers() || Global::GetConfig().IsPeerBlocked(ipAddress))
                {
                    asio::error_code ignoreError;
                    pSocket->GetAsioSocket()->shutdown(asio::socket_base::shutdown_both, ignoreError);
                    pSocket->GetAsioSocket()->close(ignoreError);

                    continue;
                }

                auto pPeer = seeder.m_peerManager.Write()->GetPeer(ipAddress);
                auto pConnection = std::make_shared<Connection>(
                    pSocket,
                    seeder.m_nextId++,
                    seeder.m_connectionManager,
                    ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
                    seeder.m_pSyncStatus,
                    seeder.m_pMessageProcessor,
                    seeder.m_peerManager
                );
                
                pConnection->Connect(); // Connection will run on its own thread.
            }
            catch (std::exception& e)
            {
                asio::error_code ignoreError;
                pSocket->GetAsioSocket()->shutdown(asio::socket_base::shutdown_both, ignoreError);
                pSocket->GetAsioSocket()->close(ignoreError);
                LOG_ERROR_F("Unable to accept connection with error: {}", e.what());
            }

            ThreadUtil::SleepFor(std::chrono::milliseconds(500));

        } while (!seeder.m_terminate && Global::IsRunning());
    }
    catch (std::exception& e)
    {
        LOG_ERROR_F("Listener failed with error: {}", e.what());
    }

    LOG_DEBUG("END");
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

    auto lastConnectTime = std::chrono::system_clock::now() - std::chrono::seconds(5);

    const size_t minimumConnections = Global::GetConfig().GetMinPeers();
    while (!seeder.m_terminate && Global::IsRunning()) {
        try {
            seeder.m_connectionManager.PruneConnections(true);

            auto now = std::chrono::system_clock::now();
            const size_t numOutbound = seeder.m_connectionManager.GetNumOutbound();

            if (numOutbound < minimumConnections && lastConnectTime + std::chrono::seconds(5) < now) {
                const size_t connectionsToAdd = (std::min)((size_t)10, (minimumConnections - numOutbound)); // Adding 10 new connections top

                for (size_t i = 0; i < connectionsToAdd; i++) {
                    seeder.SeedNewConnection();
                }

                lastConnectTime = now;
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
            asio::ip::tcp::endpoint(IPAddress::GetLocaPrimaryEndpointAddress().GetAddress(), Global::GetConfig().GetP2PPort())
        );

        m_pServerSocket = std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext);
        m_pAcceptor->async_accept(*m_pServerSocket, std::bind(&Seeder::Accept, this, std::placeholders::_1));
    }
    catch (std::exception& e) 
    {
        LOG_ERROR_F("Listener failed with error: {}", e.what());
    }
}

void Seeder::Accept(const asio::error_code& ec)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (ec) return;
    
    auto pPeer = m_peerManager.Write()->GetPeer(IPAddress::Parse(m_pServerSocket->remote_endpoint().address().to_string()));

    if (m_connectionManager.GetNumberOfActiveConnections() == Global::GetConfig().GetMaxPeers() ||
        pPeer->IsBanned() || Global::GetConfig().IsPeerBlocked(pPeer->GetIPAddress())) {
        asio::error_code ignoreError;
        m_pServerSocket->close(ignoreError);

        return;
    }

    SocketAddress socket_address = SocketAddress::FromEndpoint(m_pServerSocket->remote_endpoint());
    SocketPtr pSocket(new Socket(SocketAddress::FromEndpoint(m_pServerSocket->remote_endpoint()), m_pAsioContext, m_pServerSocket));
    
    auto pConnection = std::make_shared<Connection>(
        pSocket,
        m_nextId++,
        m_connectionManager,
        ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
        m_pSyncStatus,
        m_pMessageProcessor,
        m_peerManager
    );
    pConnection->Connect();    
    
    /*
    m_pSocket = std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext);
    m_pAcceptor->async_accept(*m_pSocket, std::bind(&Seeder::Accept, this, std::placeholders::_1));
    */
}

void Seeder::SeedNewConnection()
{
    PeerPtr pPeer = m_peerManager.Write()->GetNewPeer(Capabilities::FAST_SYNC_NODE);

    if (pPeer == nullptr) 
    {
        if (!m_usedDNS.exchange(true)) {
            std::vector<SocketAddress> peerAddresses = DNSSeeder::GetPeersFromDNS();
            m_peerManager.Write()->AddFreshPeers(peerAddresses);
        }
        return;
    }

    if (pPeer->GetIPAddress().IsLocalhost())
    {
        return;
    }

    if (m_connectionManager.ConnectionExist(pPeer->GetIPAddress()))
    {
        return;
    }

    if(m_connectionManager.IsConnected(pPeer->GetIPAddress()))
    {
        return;
    }
        
    if (!Global::GetConfig().IsPeerAllowed(pPeer->GetIPAddress())) {
        LOG_TRACE_F("peer is not allowed: {}", pPeer->GetIPAddress());
        return;
    }

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
        

    ConnectionPtr pConnection = std::make_shared<Connection>(
        pSocket,
        m_nextId++,
        m_connectionManager,
        connectedPeer,
        m_pSyncStatus,
        m_pMessageProcessor,
        m_peerManager
    );
        
    pConnection->Connect(); // Connection will run on its own thread.

    return;
}