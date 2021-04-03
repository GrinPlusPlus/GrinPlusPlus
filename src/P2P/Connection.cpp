#include "Connection.h"
#include "MessageProcessor.h"
#include "ConnectionManager.h"
#include "Messages/PingMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Seed/HandShake.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <Crypto/CSPRNG.h>
#include <thread>
#include <chrono>
#include <memory>

void Connection::Disconnect()
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    m_terminate = true;
    if (m_pSocket) {
        m_pSocket->CloseSocket();
        m_pSocket.reset();
    }
    m_connectedPeer.GetPeer()->SetConnected(false);
}

Connection::Ptr Connection::CreateInbound(
    const PeerPtr& pPeer,
    const SocketPtr& pSocket,
    const uint64_t connectionId,
    ConnectionManager& connectionManager,
    const std::weak_ptr<MessageProcessor>& pMessageProcessor,
    const SyncStatusConstPtr& pSyncStatus)
{
    auto pConnection = std::make_shared<Connection>(
        pSocket,
        connectionId,
        connectionManager,
        ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
        pSyncStatus,
        pMessageProcessor
    );
    pConnection->m_connectionThread = std::thread(Thread_ProcessConnection, pConnection);
    return pConnection;
}

Connection::Ptr Connection::CreateOutbound(
    const PeerPtr& pPeer,
    const uint64_t connectionId,
    const std::shared_ptr<asio::io_service>& pAsioContext,
    ConnectionManager& connectionManager,
    const std::weak_ptr<MessageProcessor>& pMessageProcessor,
    const SyncStatusConstPtr& pSyncStatus)
{    
    ConnectedPeer connectedPeer(
        pPeer,
        EDirection::OUTBOUND,
        Global::GetConfig().GetP2PPort()
    );
    SocketPtr pSocket(new Socket(
        connectedPeer.GetSocketAddress(),
        pAsioContext,
        std::make_shared<asio::ip::tcp::socket>(*pAsioContext)
    ));

    ConnectionPtr pConnection = std::make_shared<Connection>(
        pSocket,
        connectionId,
        connectionManager,
        connectedPeer,
        pSyncStatus,
        pMessageProcessor
    );

    pConnection->m_connectionThread = std::thread(Thread_ProcessConnection, pConnection);

    return pConnection;
}

bool Connection::IsConnectionActive() const
{
    if (m_terminate || m_pSocket == nullptr) {
        LOG_DEBUG_F("Socket has been terminated for {}", GetPeer());
        return false;
    }

    if ((m_lastReceived + std::chrono::seconds(60)) < system_clock::now()) {
        LOG_DEBUG_F("{} has not received a message in more than a minute", GetPeer());
        return false;
    }

    if (GetPeer()->IsBanned()) {
        LOG_DEBUG_F("{} is banned", GetPeer());
        return false;
    }

    return m_pSocket->IsActive();
}

void Connection::SendAsync(const IMessage& message)
{
    std::vector<uint8_t> serialized = message.Serialize(GetProtocolVersion());
    if (message.GetMessageType() != MessageTypes::Ping && message.GetMessageType() != MessageTypes::Pong) {
        LOG_TRACE_F(
            "Sending {}b '{}' message to {}",
            serialized.size(),
            MessageTypes::ToString(message.GetMessageType()),
            m_pSocket
        );
    }

    m_pSocket->SendAsync(serialized, true);
}

bool Connection::ExceedsRateLimit() const
{
    return m_pSocket->GetRateCounter().GetSentInLastMinute() > 500
        || m_pSocket->GetRateCounter().GetReceivedInLastMinute() > 500;
}

//
// Continuously checks for messages to send and/or receive until the connection is terminated.
// This function runs in its own thread.
//
void Connection::Thread_ProcessConnection(std::shared_ptr<Connection> pConnection)  // TODO: Thread no longer necessary - can be handled by asio
{
    LoggerAPI::SetThreadName("PEER");

    std::unique_lock<std::mutex> write_lock(pConnection->m_mutex);

    try {
        if (pConnection->GetDirection() == EDirection::OUTBOUND) {
            pConnection->ConnectOutbound();
        }

        pConnection->GetSocket()->SetDefaultOptions();
        HandShake(pConnection->m_connectionManager, pConnection->m_pSyncStatus)
            .PerformHandshake(*pConnection->m_pSocket, pConnection->m_connectedPeer);

        pConnection->m_lastPing = system_clock::now();
        pConnection->m_lastReceived = system_clock::now();

        LOG_DEBUG_F("Connected to {}", pConnection->m_connectedPeer);
        pConnection->GetPeer()->SetConnected(true);
        pConnection->m_connectionManager.AddConnection(pConnection);

        if (pConnection->GetDirection() == EDirection::OUTBOUND) {
            pConnection->SendSync(GetPeerAddressesMessage(Capabilities::ECapability::FAST_SYNC_NODE));
        }

        pConnection->m_received.resize(11);
        asio::async_read(
            *pConnection->GetSocket()->GetAsioSocket(),
            asio::buffer(pConnection->m_received, 11),
            std::bind(&Connection::HandleReceived, pConnection.get(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    catch (const std::exception& e) {
        LOG_ERROR_F("Failed to connect to {}: {}", pConnection->m_connectedPeer, e);
    }

    write_lock.unlock();
    ThreadUtil::Detach(pConnection->m_connectionThread);
    while (!pConnection->m_terminate && Global::IsRunning()) {
        ThreadUtil::SleepFor(std::chrono::milliseconds(20));
    }
}

void Connection::ConnectOutbound()
{
    auto pAsioSocket = m_pSocket->GetAsioSocket();
    pAsioSocket->async_connect(m_pSocket->GetEndpoint(), std::bind(&Connection::HandleConnected, this, std::placeholders::_1));

    auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(1);
    while (!m_pSocket->IsConnectFailed() && !m_pSocket->IsOpen() && std::chrono::system_clock::now() < timeout && Global::IsRunning()) {
        ThreadUtil::SleepFor(std::chrono::milliseconds(5));
    }

    if (m_pSocket->IsConnectFailed() || !m_pSocket->IsOpen()) {
        pAsioSocket->cancel();
        throw std::runtime_error("No response");
    }
}

void Connection::HandleConnected(const asio::error_code& ec)
{
    if (m_pSocket) {
        if (!ec) {
            m_pSocket->SetOpen(true);
        } else {
            m_pSocket->SetConnectFailed(true);
        }
    }
}

void Connection::HandleReceived(const asio::error_code& ec, const size_t bytes_received)
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    if (!ec) {
        if (bytes_received == 11) { // TODO: Assert?
            assert(m_received.size() == 11);

            try {
                MessageHeader msg_header = ByteBuffer(m_received).Read<MessageHeader>();
                m_received.clear();
                m_received.resize(11);

                const auto type = msg_header.GetMessageType();
                if (type == MessageTypes::Ping || type == MessageTypes::Pong) {
                    m_lastPing = system_clock::now();
                } else {
                    LOG_TRACE_F("Received '{}' from {}", msg_header, GetPeer());
                }

                std::vector<uint8_t> payload;
                const bool bPayloadRetrieved = m_pSocket->Receive(
                    msg_header.GetMessageLength(),
                    false,
                    Socket::BLOCKING,
                    payload
                );
                if (bPayloadRetrieved) {
                    GetPeer()->UpdateLastContactTime();
                    m_lastReceived = std::chrono::system_clock::now();

                    auto pMessageProcessor = m_pMessageProcessor.lock();
                    if (pMessageProcessor != nullptr) {
                        RawMessage message(std::move(msg_header), std::move(payload));
                        pMessageProcessor->ProcessMessage(*this, message);
                    }

                    asio::async_read(
                        *m_pSocket->GetAsioSocket(),
                        asio::buffer(m_received, 11),
                        std::bind(&Connection::HandleReceived, this, std::placeholders::_1, std::placeholders::_2)
                    );

                    return;
                }

                LOG_INFO_F("Expected payload not received from: {}", GetPeer());
            }
            catch (std::exception& e) {
                LOG_INFO_F("Error while receiving from {}: {}", GetPeer(), e.what());
            }
        }
    }

    LOG_INFO_F("Disconnecting from socket {}. Error: {}", GetSocket(), ec ? ec.message() : "<none>");
    m_pSocket->CloseSocket();
    GetPeer()->SetConnected(false);
}

void Connection::CheckPing()
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    if (GetSocket() == nullptr || !GetSocket()->IsActive()) {
        return;
    }

    if (ExceedsRateLimit()) {
        LOG_WARNING_F("Banning {} for exceeding rate limit.", GetPeer());
        GetPeer()->Ban(EBanReason::Abusive);
        return;
    }

    if (m_lastPing + std::chrono::seconds(10) < system_clock::now()) {
        uint64_t block_difficulty = m_pSyncStatus->GetBlockDifficulty();
        uint64_t block_height = m_pSyncStatus->GetBlockHeight();
        SendAsync(PingMessage{ block_difficulty, block_height });

        m_lastPing = system_clock::now();
    }
}

bool Connection::SendSync(const IMessage& message)
{
    std::vector<uint8_t> serialized_message = message.Serialize(GetProtocolVersion());
    if (message.GetMessageType() != MessageTypes::Ping && message.GetMessageType() != MessageTypes::Pong) {
        LOG_TRACE_F(
            "Sending {}b '{}' message to {}",
            serialized_message.size(),
            MessageTypes::ToString(message.GetMessageType()),
            GetSocket()
        );
    }

    return GetSocket()->Send(serialized_message, true);
}

void Connection::BanPeer(const EBanReason reason)
{
    LOG_WARNING_F("Banning peer {} for '{}'.", GetIPAddress(), BanReason::Format(reason));
    GetPeer()->Ban(reason);
    m_terminate = true;
}