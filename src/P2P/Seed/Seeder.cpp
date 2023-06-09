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

#include <algorithm>
#include <random>


Seeder::~Seeder() {
    LOG_INFO("Shutting down seeder");
    m_terminate = true;

    m_pAcceptor->cancel();
    m_pAcceptor->close();
    m_pAsioContext->stop();

    std::unique_lock<std::mutex> lock(m_mutex);

    ThreadUtil::Join(m_listenerThread);
    ThreadUtil::Join(m_seedThread);
    ThreadUtil::Join(m_prunerThread);
}

std::unique_ptr<Seeder> Seeder::Create(ConnectionManager& connectionManager,
    Locked<PeerManager> peerManager,
    const IBlockChain::Ptr& pBlockChain,
    std::shared_ptr<Pipeline> pPipeline,
    SyncStatusConstPtr pSyncStatus)
{
    auto pMessageProcessor = std::make_shared<MessageProcessor>(
        connectionManager, peerManager, pBlockChain, pPipeline, pSyncStatus);
    std::unique_ptr<Seeder> pSeeder(new Seeder(connectionManager, peerManager,
        pBlockChain, pMessageProcessor,
        pSyncStatus));

    pSeeder->m_listenerThread = std::thread(Thread_Listener, std::ref(*pSeeder.get()));
    pSeeder->m_seedThread = std::thread(Thread_Seed, std::ref(*pSeeder.get()));
    pSeeder->m_prunerThread = std::thread(Thread_Pruner, std::ref(*pSeeder.get()));

    return pSeeder;
}

void Seeder::Thread_Listener(Seeder& seeder)
{
    LoggerAPI::SetThreadName("LISTENER");
    LOG_DEBUG("BEGIN");

    asio::error_code errorCode;

    try {
        const size_t inboundsAllowed = Global::GetConfig().GetMaxPeers() - Global::GetConfig().GetMinPeers();

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

        do {
            SocketPtr pSocket(new Socket(
                SocketAddress::FromEndpoint(endpoint), seeder.m_pAsioContext,
                std::make_shared<asio::ip::tcp::socket>(*seeder.m_pAsioContext)));

            try {
                seeder.m_pAcceptor->accept(*pSocket->GetAsioSocket());

                auto ipAddress =
                    IPAddress(pSocket->GetAsioSocket()->remote_endpoint().address());

                if (Global::GetConfig().IsPeerBlocked(ipAddress) ||
                    seeder.m_connectionManager.GetNumInbound() < inboundsAllowed)
                {
                    asio::error_code ignoreError;
                    pSocket->GetAsioSocket()->shutdown(asio::socket_base::shutdown_both, ignoreError);
                    pSocket->GetAsioSocket()->close(ignoreError);

                    continue;
                }

                auto pPeer = seeder.m_peerManager.Write()->GetPeer(ipAddress);
                auto pConnection = std::make_shared<Connection>(
                    pSocket, seeder.m_nextId++, seeder.m_connectionManager,
                    ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
                    seeder.m_pSyncStatus, seeder.m_pMessageProcessor,
                    seeder.m_peerManager);

                pConnection->Connect();  // Connection will run on its own thread.
            }
            catch (std::exception& e) {
                asio::error_code ignoreError;
                pSocket->GetAsioSocket()->shutdown(asio::socket_base::shutdown_both,
                    ignoreError);
                pSocket->GetAsioSocket()->close(ignoreError);
                LOG_ERROR_F("Unable to accept connection with error: {}", e.what());
            }

            ThreadUtil::SleepFor(std::chrono::milliseconds(1000));
            
        } while (!seeder.m_terminate && Global::IsRunning());
    }
    catch (std::exception& e)
    {
        LOG_ERROR_F("Listener failed with error: {}", e.what());
    }

    LOG_DEBUG("END");
}

//
// Continuously checks the number of connected peers, and connects to additional
// peers when the number of connections drops below the minimum.
// This function operates in its own thread.
//
void Seeder::Thread_Seed(Seeder& seeder) 
{
    LoggerAPI::SetThreadName("SEED");
    LOG_TRACE("BEGIN");

    const size_t outboundsAllowed = Global::GetConfig().GetMinPeers();
    do 
    {
        try 
        {
            const size_t freeSlots = outboundsAllowed - seeder.m_connectionManager.GetNumOutbound();
            if (freeSlots > 0)
            {
                seeder.SeedConnections(freeSlots, outboundsAllowed);
            }
        }
        catch (std::exception& e) {
            LOG_WARNING_F("Exception thrown: {}", e.what());
        }
        ThreadUtil::SleepFor(std::chrono::seconds(5));
    } while (!seeder.m_terminate && Global::IsRunning());

    LOG_TRACE("END");
}

void Seeder::SeedConnections(const size_t connections, const size_t max) 
{
    try
    {
        auto set = Global::GetConfig().GetPreferredPeers();
        for (auto it = set.begin(); it != set.end(); )
        {
            if (m_terminate || !Global::IsRunning() || m_connectionManager.GetNumOutbound() >= max)
            {
                return;
            }
            
            auto pPeer = m_peerManager.Write()->GetPeer(set.extract(it++).value());
            ConnectPeer(pPeer);
            
            ThreadUtil::SleepFor(std::chrono::milliseconds(500));
        }
    }
    catch ( ... ) { }
    
    if (m_connectionManager.GetNumOutbound() >= max)
    {
        return;
    }
    
    auto pPeers = m_peerManager.Write()->GetPeers(Capabilities::UNKNOWN, (uint16_t)connections);
    for (auto pPeer : pPeers)
    {
        if (m_terminate || !Global::IsRunning() || m_connectionManager.GetNumOutbound() >= max)
        {
            return;
        }
        ConnectPeer(pPeer);
        ThreadUtil::SleepFor(std::chrono::milliseconds(250));
    }
}

void Seeder::ConnectPeer(PeerPtr& pPeer) 
{
    if (m_connectionManager.ConnectionExist(pPeer->GetIPAddress()) ||
        m_connectionManager.IsConnected(pPeer->GetIPAddress()) ||
        pPeer->IsBanned() ||
        pPeer->GetIPAddress().IsLocalhost() ||
        !Global::GetConfig().IsPeerAllowed(pPeer->GetIPAddress()) ||
        m_terminate ||
        !Global::IsRunning())
    {
        return;
    }

    LOG_TRACE_F("Attempting to seed with: {}", pPeer);

    ConnectedPeer connectedPeer(pPeer, EDirection::OUTBOUND,
        Global::GetConfig().GetP2PPort());

    SocketPtr pSocket(
        new Socket(connectedPeer.GetSocketAddress(), m_pAsioContext,
            std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext)));

    ConnectionPtr pConnection = std::make_shared<Connection>(
            pSocket, m_nextId++, m_connectionManager, connectedPeer,
            m_pSyncStatus,m_pMessageProcessor, m_peerManager);

    pConnection->Connect();  // Connection will run on its own thread.
}

void Seeder::Thread_Pruner(Seeder& seeder)
{
    do
    {
        try
        {
            ThreadUtil::SleepFor(std::chrono::seconds(60));
            seeder.m_connectionManager.PruneConnections(true);
            LOG_TRACE("Checking inactive connections...");
        }
        catch (std::exception& e) {
            LOG_TRACE_F("Exception thrown: {}", e.what());
        }
    } while (!seeder.m_terminate && Global::IsRunning());
}