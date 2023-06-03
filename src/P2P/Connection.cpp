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

static const std::chrono::seconds PING_THRESHOLD_SECONDS = std::chrono::seconds(10);

void Connection::Connect()
{
    std::thread connect_thr(Thread_Connect, shared_from_this());
    ThreadUtil::Detach(connect_thr);
}

void Connection::Disconnect()
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    if (!m_terminate) {
        m_terminate = true;
        GetSocket()->CloseSocket();
        m_connectedPeer.GetPeer()->SetConnected(false);
    }
}

bool Connection::IsConnectionActive() const
{
    if (m_terminate) {
        LOG_DEBUG_F("Socket has been terminated for {}", GetPeer());
        return false;
    }

    if ((m_lastReceived + std::chrono::seconds(120)) < system_clock::now()) {
        LOG_DEBUG_F("{} has not received a message in more than a minute", GetPeer());
        return false;
    }

    if (GetPeer()->IsBanned()) {
        LOG_DEBUG_F("{} is banned", GetPeer());
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
        pConnection->Init();

        pConnection->IsBusy(true);
        HandShake handShake(pConnection->m_pSyncStatus, pConnection->GetSocket());
        handShake.Perform(pConnection->m_connectedPeer.GetDirection());
        handShake.UpdateConnectedPeer(pConnection->m_connectedPeer);
        pConnection->IsBusy(false);
        
        pConnection->IsBusy(true);
        CapabilitiesExchanger capabilitiesExchanger = CapabilitiesExchanger(pConnection->m_peerManager);
        capabilitiesExchanger.Perform(pConnection->GetSocket(), pConnection->m_connectedPeer);
        pConnection->IsBusy(false);

        pConnection->m_lastPing = system_clock::now();
        pConnection->m_lastReceived = pConnection->m_lastPing;

        pConnection->GetPeer()->SetConnected(true);

        pConnection->m_connectionManager.AddConnection(pConnection);

        LOG_DEBUG_F("Connected to {}", pConnection->m_connectedPeer);
    }
    catch (const std::exception& e) {
        pConnection->GetPeer()->SetConnected(false);
        pConnection->GetSocket()->SetOpen(false);
        pConnection->GetSocket()->SetConnectFailed(true);
        if (pConnection->GetSocket()->CloseSocket())
        {
            LOG_DEBUG_F("Socket with {}:{} closed.", pConnection->m_connectedPeer, pConnection->m_connectedPeer.GetPort());
        }
        LOG_ERROR_F("Failed to connect to {}:{} -> {}", pConnection->m_connectedPeer, pConnection->m_connectedPeer.GetPort(), e);
    }
}

void Connection::Init()
{
    IsBusy(true);

    if (GetDirection() == EDirection::OUTBOUND)
    {
        asio::error_code err;
        GetSocket()->GetAsioSocket()->open(GetSocket()->GetEndpoint().protocol());
        GetSocket()->GetAsioSocket()->connect(GetSocket()->GetEndpoint(), err);

        if (err || err == asio::error::eof) // there is no error
        {
            throw std::runtime_error(err.message());
        }
    }
    GetSocket()->SetBlocking(true);
    GetSocket()->SetDelayOption(true);

    GetSocket()->SetOpen(true);
    GetSocket()->SetConnectFailed(false);

    IsBusy(false);
}

void Connection::ConnectOutbound()
{
    asio::error_code err;
    GetSocket()->GetAsioSocket()->connect(GetSocket()->GetEndpoint(), err);
    if (!err) // there is no error
    {
        GetSocket()->SetOpen(true);
    }
    else {
        LOG_DEBUG_F("Error connecting to Socket {} with error: {}", GetSocket()->GetIPAddress(), err.message());
        GetSocket()->SetConnectFailed(true);
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

void Connection::CheckPingSync()
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    if (IsBusy())
    {
        return; 
    }

    if (!GetSocket()->IsActive())
    {
        return;
    }

    if (ExceedsRateLimit())
    {
        LOG_WARNING_F("Banning {} for exceeding rate limit.", GetPeer());
        GetPeer()->Ban(EBanReason::Abusive);
        return;
    }
    
    if (m_lastPing + PING_THRESHOLD_SECONDS > system_clock::now())
    {
        return;
    }

    try
    {
        IsBusy(true);
        const PongMessage pongMessage = PingPong(m_pSyncStatus).Execute(GetSocket(), m_connectedPeer);
        UpdateTotals(pongMessage.GetTotalDifficulty(), pongMessage.GetHeight());
        m_lastPing = system_clock::now();
        m_lastReceived = m_lastPing;
        IsBusy(false);
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR_F("Unable to Ping to {}: {} ({})", m_connectedPeer, e, m_connectedPeer.GetPeer()->GetUserAgent());
    }
}

void Connection::SendAsync(const IMessage& message)
{
    if (!IsBusy()) {
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
    IsBusy(true);

    std::vector<uint8_t> serialized_message = message.Serialize(GetProtocolVersion());
    
    LOG_DEBUG_F(
        "Sending {}b '{}' message to {}",
        serialized_message.size(),
        MessageTypes::ToString(message.GetMessageType()),
        GetSocket()
    );

    IsBusy(false);

    bool sent = GetSocket()->SendSync(serialized_message, true);
    
    if (sent)
    {
        LOG_DEBUG_F(
            "Sent {}b of '{}' message to {}",
            serialized_message.size(),
            MessageTypes::ToString(message.GetMessageType()),
            GetSocket()
        );
        return true;
    }
    LOG_ERROR_F(
        "Unable to send {}b of '{}' message to {}",
        serialized_message.size(),
        MessageTypes::ToString(message.GetMessageType()),
        GetSocket()
    );
    return false;
}

bool Connection::ReceiveSync(std::vector<uint8_t>& bytes, const size_t num_bytes)
{
    std::unique_lock<std::mutex> write_lock(m_mutex);

    bytes = GetSocket()->ReceiveSync(num_bytes, false);
    if (bytes.size() != num_bytes) {
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
    IsBusy(true);

    std::vector<uint8_t> headerBuffer = GetSocket()->ReceiveSync(P2P::HEADER_LENGTH, true);

    if (headerBuffer.size() != P2P::HEADER_LENGTH) {
        return nullptr;
    }

    MessageHeader messageHeader = ByteBuffer(std::move(headerBuffer)).Read<MessageHeader>();
    
    std::vector<uint8_t> payload = GetSocket()->ReceiveSync(messageHeader.GetMessageLength(), false);
    if (payload.size() != messageHeader.GetMessageLength())
    {
        throw std::runtime_error("Expected payload not received");
    }

    IsBusy(false);
    
    return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}

std::unique_ptr<RawMessage> Connection::SendReceiveSync(const IMessage& message)
{
    const bool messageSent = SendSync(message);
    if (!messageSent)
    {
        return nullptr;
    }

    m_connectedPeer.GetPeer()->UpdateLastContactTime();
    m_lastPing = system_clock::now();

    std::unique_ptr<RawMessage> rawMessage = RetrieveMessage();
    if (rawMessage == nullptr)
    {
        return nullptr;
    }
    LOG_DEBUG_F("Received '{}' message from {}", rawMessage->GetMessageHeader(), m_connectedPeer.GetPeer());

    m_connectedPeer.GetPeer()->UpdateLastContactTime();
    m_lastReceived = system_clock::now();

    auto pMessageProcessor = m_pMessageProcessor.lock();
    if (pMessageProcessor == nullptr)
    {
        return nullptr;
    }
    
    pMessageProcessor->ProcessMessage(shared_from_this(), *rawMessage);
    LOG_DEBUG_F("Message '{}' from '{}' will be processed...", rawMessage->GetMessageHeader(), m_connectedPeer.GetPeer());

    return rawMessage;
}


bool Connection::ReceiveProcessSync()
{
    std::unique_ptr<RawMessage> rawMessage = RetrieveMessage();
    if (rawMessage != nullptr)
    {
        m_lastPing = system_clock::now();
        m_lastReceived = m_lastPing;

        auto pMessageProcessor = m_pMessageProcessor.lock();
        if (pMessageProcessor != nullptr) {
            pMessageProcessor->ProcessMessage(shared_from_this(), *rawMessage);
        }

        return true;
    }

    return false;
}