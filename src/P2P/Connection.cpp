#include "Connection.h"
#include "MessageProcessor.h"
#include "ConnectionManager.h"

#include "Messages/PingMessage.h"
#include "Messages/GetPeerAddressesMessage.h"

#include "Seed/HandShake.h"
#include "Seed/CapabilitiesExchanger.h"
#include "Seed/PingPong.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>

#include <thread>
#include <chrono>
#include <memory>
#include "Messages/PongMessage.h"

static const std::chrono::seconds PING_THRESHOLD_SECONDS = std::chrono::seconds(5);

void Connection::Connect()
{
    std::thread connect_thr(Thread_Connect, shared_from_this());
    ThreadUtil::Detach(connect_thr);
}

void Connection::Disconnect()
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    m_terminate = true;
    m_connectedPeer.GetPeer()->SetConnected(false);
    GetSocket()->CloseSocket();
    GetSocket()->SetOpen(false);
}

bool Connection::IsConnectionActive()
{
    if (m_terminate) 
    {
        LOG_DEBUG_F("Socket has been terminated for Peer: {}", GetPeer());
        return false;
    }

    if (GetPeer()->IsBanned())
    {
        LOG_DEBUG_F("{} is banned", GetPeer());
        return false;
    }

    if ((m_lastReceived + std::chrono::seconds(600)) < system_clock::now() && !IsBusy()) 
    {
        LOG_DEBUG_F("{} has not received a message in more than 10 minutes", GetPeer());
        return false;
    }

    return GetSocket()->IsActive();
}

bool Connection::ExceedsRateLimit() const
{
    return GetSocket()->GetRateCounter().GetSentInLastMinute() > 500
        || GetSocket()->GetRateCounter().GetReceivedInLastMinute() > 500;
}

void Connection::Thread_Connect(std::shared_ptr<Connection> pConnection)
{
    LoggerAPI::SetThreadName("PEER");
    try {
        HandShake handShake(pConnection->m_pSyncStatus, pConnection->GetSocket());
        
        EDirection direction = pConnection->m_connectedPeer.GetDirection();
        switch (direction) {
        case EDirection::OUTBOUND:
        {
            pConnection->ConnectOutbound();
            handShake.Perform(pConnection->m_connectedPeer, direction);
            const PingMessage pingMessage = PingMessage(pConnection->m_pSyncStatus->GetBlockDifficulty(), pConnection->m_pSyncStatus->GetBlockHeight());
            pConnection->SendReceiveSync(pingMessage);
            try
            {
                CapabilitiesExchanger(pConnection->m_peerManager).Perform(pConnection->GetSocket(), pConnection->m_connectedPeer);
            }
            catch (...) {}
            break;
        }
        case EDirection::INBOUND:
        {
            handShake.Perform(pConnection->m_connectedPeer, direction);
            break;
        }
        default:
        {
            break;
        }
        }

        pConnection->GetSocket()->SetDefaults();

        pConnection->GetSocket()->SetOpen(true);
        pConnection->GetSocket()->SetConnectFailed(false);

        pConnection->GetPeer()->SetConnected(true);
        pConnection->m_connectionManager.AddConnection(pConnection);
    }
    catch (const std::exception& e) 
    {
        pConnection->Disconnect();
        LOG_TRACE_F("Failed to connect to {} -> {}", pConnection->m_connectedPeer, e);
    }
}

void Connection::ConnectOutbound()
{
    asio::error_code err;
    GetSocket()->GetAsioSocket()->open(GetSocket()->GetEndpoint().protocol());
    GetSocket()->GetAsioSocket()->connect(GetSocket()->GetEndpoint(), err);

    if (err || err == asio::error::eof) // there is no error
    {
        throw std::runtime_error(err.message());
    }
}

void Connection::HandleConnected(const asio::error_code& ec)
{
    if (GetSocket()) {
        if (!ec) {
            GetSocket()->SetOpen(true);
        } else {
            LOG_DEBUG_F("{} Socket error: {}", GetSocket()->GetIPAddress(), ec.message());
            GetSocket()->SetConnectFailed(true);
        }
    }
}

void Connection::HandleReceivedHeader(const asio::error_code& ec, const size_t bytes_received)
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    if (!ec  && bytes_received == P2P::HEADER_LENGTH) {
        assert(m_received.size() == P2P::HEADER_LENGTH);

        try {
            MessageHeader msg_header = ByteBuffer(m_received).Read<MessageHeader>();

            m_received.clear();
            m_received.resize(msg_header.GetLength());
            asio::async_read(
                *GetSocket()->GetAsioSocket(),
                asio::buffer(m_received, m_received.size()),
                std::bind(&Connection::HandleReceivedBody, shared_from_this(), msg_header, std::placeholders::_1, std::placeholders::_2)
            );

            return;
        }
        catch (std::exception& e) {
            LOG_INFO_F("Error while receiving from {}: {}", GetPeer(), e.what());
        }
    }

    LOG_INFO_F("Disconnecting from socket {}. Error: {}", GetSocket(), ec ? ec.message() : "<none>");
    GetSocket()->CloseSocket();
    GetPeer()->SetConnected(false);
}

void Connection::HandleReceivedBody(MessageHeader msg_header, const asio::error_code& ec, const size_t bytes_received)
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    if (!ec && bytes_received == msg_header.GetLength()) {
        assert(m_received.size() == msg_header.GetLength());

        try {
            const auto type = msg_header.GetMessageType();
            if (type == MessageTypes::Ping || type == MessageTypes::Pong) {
                m_lastPing = system_clock::now();
            } else {
                LOG_TRACE_F("Received '{}' from {}", msg_header, GetPeer());
            }

            GetPeer()->UpdateLastContactTime();
            m_lastPing = std::chrono::system_clock::now();
            m_lastReceived = m_lastPing;

            auto pMessageProcessor = m_pMessageProcessor.lock();
            if (pMessageProcessor != nullptr) {
                RawMessage message(std::move(msg_header), std::move(m_received));
                pMessageProcessor->ProcessMessage(shared_from_this(), message);
            }

            if (!m_receivingDisabled) {
                m_received.clear();
                m_received.resize(P2P::HEADER_LENGTH);
                asio::async_read(
                    *GetSocket()->GetAsioSocket(),
                    asio::buffer(m_received, m_received.size()),
                    std::bind(&Connection::HandleReceivedHeader, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
                );
            }

            return;
        }
        catch (std::exception& e) {
            LOG_INFO_F("Error while receiving from {}: {}", GetPeer(), e.what());
        }
    }

    LOG_INFO_F("Disconnecting from socket {}. Error: {}", GetSocket(), ec ? ec.message() : "<none>");
    GetSocket()->CloseSocket();
    GetPeer()->SetConnected(false);
}

bool Connection::ShouldBePinged()
{
    return system_clock::now() < m_lastPing + PING_THRESHOLD_SECONDS;
}

void Connection::PingSync()
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    try
    {
        if (GetSocket()->HasReceivedData() && ReceiveProcessSync())
        {
            LOG_TRACE_F("Socket read before sending ping: {}", m_connectedPeer);
        }
    }
    catch (...) { }

    try
    {
        const PingMessage ping = PingMessage(m_pSyncStatus->GetBlockDifficulty(), m_pSyncStatus->GetBlockHeight());
        const auto response = SendReceiveSync(ping);
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR_F("Unable to Ping to {}: {} ({})", m_connectedPeer, e, m_connectedPeer.GetPeer()->GetUserAgent());
    }
}

void Connection::SendAsync(const IMessage& message)
{
    if (!IsBusy()) 
    {
        std::vector<uint8_t> serialized = message.Serialize(GetProtocolVersion());
        if (message.GetMessageType() != MessageTypes::Ping && message.GetMessageType() != MessageTypes::Pong) {
            LOG_TRACE_F(
                "Sending {}b '{}' message to {}",
                serialized.size(),
                MessageTypes::ToString(message.GetMessageType()),
                GetSocket()
            );
        }

        GetSocket()->SendAsync(serialized);
    }
}

bool Connection::SendSync(const IMessage& message)
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    std::vector<uint8_t> serializedMessage = message.Serialize(GetProtocolVersion());
    
    bool sent = false;
    
    try
    {
        sent = GetSocket()->SendSync(serializedMessage, true);
    }
    catch (const std::exception& e) 
    {
        LOG_INFO_F("Error sending to {}:'{}'.", GetIPAddress(), e.what());
        return false;
    }

    if (!sent)
    {
        LOG_TRACE_F(
            "Unable to send {}b of '{}' message to {}",
            serializedMessage.size(),
            MessageTypes::ToString(message.GetMessageType()),
            GetSocket()
        );
        return false;
    }

    LOG_TRACE_F(
        "Sent {}b of '{}' message to {}",
        serializedMessage.size(),
        MessageTypes::ToString(message.GetMessageType()),
        GetSocket()
    );
    
    m_lastPing = system_clock::now();
    m_connectedPeer.GetPeer()->UpdateLastContactTime();

    return true;
}

bool Connection::ReceiveSync(std::vector<uint8_t>& bytes, const size_t num_bytes)
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    m_lastPing = system_clock::now();
    
    try
    {
        bytes = GetSocket()->ReceiveSync(num_bytes, false);
    }
    catch (const std::exception& e) {
        LOG_INFO_F("Error receiving from {}:'{}'.", GetIPAddress(), e.what());
        return false;
    }

    if (bytes.size() != num_bytes) 
    { 
        return false;
    }

    m_lastReceived = system_clock::now();
    GetPeer()->UpdateLastContactTime();

    return true;
}

void Connection::BanPeer(const EBanReason reason)
{
    LOG_WARNING_F("Banning peer {} for '{}'.", GetIPAddress(), BanReason::Format(reason));
    GetPeer()->Ban(reason);
    m_terminate = true;
}

std::unique_ptr<RawMessage> Connection::RetrieveMessage()
{
    // first, let's read the message header
    std::vector<uint8_t> headerBuffer = GetSocket()->ReceiveSync(P2P::HEADER_LENGTH, true);

    if (headerBuffer.size() != P2P::HEADER_LENGTH) {
        throw std::runtime_error("Expected header not received");
    }

    MessageHeader messageHeader = ByteBuffer(std::move(headerBuffer)).Read<MessageHeader>(); // now we have the payload size
    
    // let's read the payload
    std::vector<uint8_t> payload = GetSocket()->ReceiveSync(messageHeader.GetMessageLength(), false);
    if (payload.size() != messageHeader.GetMessageLength())
    {
        throw std::runtime_error("Expected payload not received");
    }

    m_lastReceived = m_lastPing;
    GetPeer()->UpdateLastContactTime();

    return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}

std::unique_ptr<RawMessage> Connection::SendReceiveSync(const IMessage& message)
{
    if (IsBusy())
    {
        return nullptr; 
    }
    std::unique_ptr<RawMessage> rawMessage = nullptr;

    IsBusy(true);
    try
    {
        const bool messageSent = SendSync(message);
        if (!messageSent)
        {
            return nullptr;
        }

        rawMessage = RetrieveMessage();
        if (rawMessage == nullptr)
        {
            return nullptr;
        }

        auto pMessageProcessor = m_pMessageProcessor.lock();
        if (pMessageProcessor == nullptr)
        {
            return nullptr;
        }

        pMessageProcessor->ProcessMessage(shared_from_this(), *rawMessage);
        LOG_TRACE_F("Message '{}' from '{}' will be processed...", rawMessage->GetMessageHeader(), m_connectedPeer.GetPeer());
    } catch ( ... ) { }
    IsBusy(false);

    return rawMessage;
}

bool Connection::ReceiveProcessSync()
{
    std::unique_ptr<RawMessage> rawMessage = RetrieveMessage();
    if (rawMessage ==  nullptr)
    {
        return false;
    }
 
    auto pMessageProcessor = m_pMessageProcessor.lock();
    if (pMessageProcessor == nullptr) 
    {
        return false;
    }

    pMessageProcessor->ProcessMessage(shared_from_this(), *rawMessage);

    return true;
}