#include "Connection.h"
#include "MessageRetriever.h"
#include "MessageProcessor.h"
#include "ConnectionManager.h"
#include "Messages/PingMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Seed/HandShake.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <thread>
#include <chrono>
#include <memory>

void Connection::Disconnect(const bool wait)
{
    m_terminate = true;
    if (wait) {
        ThreadUtil::Join(m_connectionThread);
        m_connectedPeer.GetPeer()->SetConnected(false);
        m_pSocket.reset();
    }
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
        Global::GetConfig(),
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
    const Config& config,
    ConnectionManager& connectionManager,
    const std::weak_ptr<MessageProcessor>& pMessageProcessor,
    const SyncStatusConstPtr& pSyncStatus)
{
    ConnectedPeer connectedPeer(pPeer, EDirection::OUTBOUND, config.GetP2PPort());
    SocketPtr pSocket = std::make_shared<Socket>(
        SocketAddress{ pPeer->GetIPAddress(), config.GetP2PPort() }
    );
    ConnectionPtr pConnection = std::make_shared<Connection>(
        config,
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
        return false;
    }

    if ((m_lastReceived + std::chrono::seconds(60)) < system_clock::now()) {
        return false;
    }

    if (GetPeer()->IsBanned()) {
        return false;
    }

    return m_pSocket->IsActive();
}

void Connection::AddToSendQueue(const IMessage& message)
{
    m_sendQueue.push_back(message.Clone());
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
void Connection::Thread_ProcessConnection(std::shared_ptr<Connection> pConnection)
{
    LoggerAPI::SetThreadName("PEER");

    try {
        if (pConnection->Connect()) {
            LOG_DEBUG_F("Connected to {}", pConnection->m_connectedPeer);

            pConnection->GetPeer()->SetConnected(true);
            pConnection->m_connectionManager.AddConnection(pConnection);
            pConnection->SendMsg(GetPeerAddressesMessage(Capabilities::ECapability::FAST_SYNC_NODE));
            pConnection->Run();
            pConnection->GetSocket()->CloseSocket();
            pConnection->GetPeer()->SetConnected(false);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR_F("Failed to connect to {}: {}", pConnection->m_connectedPeer, e);
    }

    pConnection->m_terminate = true;
    ThreadUtil::Detach(pConnection->m_connectionThread);
}

bool Connection::Connect()
{
    EDirection direction = m_pSocket->IsSocketOpen() ? EDirection::INBOUND : EDirection::OUTBOUND;
    if (direction == EDirection::OUTBOUND) {
        if (!m_pSocket->Connect(std::make_shared<asio::io_context>())) {
            return false;
        }
    }

    HandShake(m_connectionManager, m_pSyncStatus)
        .PerformHandshake(*m_pSocket, m_connectedPeer, direction);

    m_lastPing = system_clock::now();
    m_lastReceived = system_clock::now();

    return true;
}

void Connection::Run()
{
    while (!GetPeer()->IsBanned() && IsConnectionActive()) {
        if (ExceedsRateLimit()) {
            LOG_WARNING_F("Banning peer ({}) for exceeding rate limit.", GetIPAddress());
            GetPeer()->Ban(EBanReason::Abusive);
            break;
        }

        try {
            CheckPing();

            bool message_sent = CheckSend();
            bool message_received = CheckReceive();
            if (!message_sent && !message_received) {
                ThreadUtil::SleepFor(std::chrono::milliseconds(5));
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR_F("Exception thrown: {}", e.what());
            break;
        }
    }

    LOG_INFO_F("Disconnecting from peer {}", GetPeer());
}

void Connection::CheckPing()
{
    if (m_lastPing + std::chrono::seconds(10) < system_clock::now()) {
        uint64_t block_difficulty = m_pSyncStatus->GetBlockDifficulty();
        uint64_t block_height = m_pSyncStatus->GetBlockHeight();
        AddToSendQueue(PingMessage{ block_difficulty, block_height });

        m_lastPing = system_clock::now();
    }
}

// Send the next message in the queue, if one exists.
bool Connection::CheckSend()
{
    auto pMessageToSend = m_sendQueue.copy_front();
    if (pMessageToSend != nullptr) {
        IMessagePtr pMessage = *pMessageToSend;
        m_sendQueue.pop_front(1);
        SendMsg(*pMessage);
    }

    return pMessageToSend != nullptr;
}

// Check for received messages and if there is a new message, process it.
bool Connection::CheckReceive()
{
    std::unique_ptr<RawMessage> pRawMessage = MessageRetriever::RetrieveMessage(
        *m_pSocket,
        *GetPeer(),
        Socket::NON_BLOCKING
    );

    if (pRawMessage != nullptr) {
        auto pMessageProcessor = m_pMessageProcessor.lock();
        if (pMessageProcessor != nullptr) {
            pMessageProcessor->ProcessMessage(*this, *pRawMessage);
        }

        m_lastReceived = system_clock::now();
    }

    return pRawMessage != nullptr;
}

bool Connection::SendMsg(const IMessage& message)
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