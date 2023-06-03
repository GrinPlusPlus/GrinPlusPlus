#include "HandShake.h"

#include "../ConnectionManager.h"

#include "../Messages/HandMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/BanReasonMessage.h"

#include <Core/Exceptions/ProtocolException.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>
#include <P2P/Direction.h>


static const uint64_t SELF_NONCE = CSPRNG::GenerateRandom(0, UINT64_MAX);

void HandShake::Perform(EDirection direction)
{
    LOG_DEBUG_F(
        "Performing handshake with {}({})",
        m_pSocket,
        (direction == EDirection::INBOUND ? "inbound" : "outbound")
    );
    
    switch (direction) {
        case EDirection::OUTBOUND:
        {
            // Send Hand Message
            TransmitHandMessage();
            // Get Shake Message
            ShakeMessage shakeMessage = RetrieveShakeMessage();
            break;
        }
        case EDirection::INBOUND:
        {
            // Get Hand Message
            HandMessage hand_message = RetrieveHandMessage();
            // Send Shake Message
            TransmitShakeMessage();
            break;
        }
        default:
        {
            break;
        }
    }
}


void HandShake::TransmitHandMessage()
{
    IPAddress localHostIP = Global::GetConfig().GetP2PIP();

    HandMessage hand(
        P2P::PROTOCOL_VERSION,
        Capabilities::FAST_SYNC_NODE,
        SELF_NONCE,
        Global::GetGenesisHash(),
        m_pSyncStatus->GetBlockDifficulty(),
        SocketAddress(localHostIP, Global::GetConfig().GetP2PPort()),
        SocketAddress(m_pSocket->GetIPAddress(), m_pSocket->GetPort()),
        P2P::USER_AGENT
    );

    if (!m_pSocket->SendSync(hand.Serialize(ProtocolVersion::Local()), true))
    {
        throw PROTOCOL_EXCEPTION("Failed to send hand message.");
    }
}

HandMessage HandShake::RetrieveHandMessage()
{
    auto pReceived = RetrieveMessage();
    if (pReceived == nullptr)
    {
        throw PROTOCOL_EXCEPTION("Hand message not received.");
    }

    if (pReceived->GetMessageType() != MessageTypes::Hand)
    {
        throw PROTOCOL_EXCEPTION_F("Expected hand but received {}", *pReceived);
    }

    ByteBuffer byteBuffer(pReceived->GetPayload());
    HandMessage hand = HandMessage::Deserialize(byteBuffer);

    m_Version = (std::min)(P2P::PROTOCOL_VERSION, hand.GetVersion());
    m_Capabilities = hand.GetCapabilities();
    m_UserAgent = hand.GetUserAgent();
    m_TotalDifficulty = hand.GetTotalDifficulty();

    return hand;
}

void HandShake::TransmitShakeMessage()
{
    ShakeMessage shakeMessage(
        m_Version,
        Capabilities::FAST_SYNC_NODE,
        Global::GetGenesisHash(),
        m_pSyncStatus->GetBlockDifficulty(),
        P2P::USER_AGENT
    );

    if (!m_pSocket->SendSync(shakeMessage.Serialize(ProtocolVersion::ToEnum(m_Version)), true))
    {
        throw PROTOCOL_EXCEPTION("Failed to send shake message.");
    }
}

ShakeMessage HandShake::RetrieveShakeMessage()
{
    auto pReceived = RetrieveMessage();
    if (pReceived == nullptr)
    {
        throw PROTOCOL_EXCEPTION_F("Shake message not received ({})", m_pSocket->GetIPAddress());
    }

    if (pReceived->GetMessageType() != MessageTypes::Shake)
    {
        throw PROTOCOL_EXCEPTION_F("Expected shake but received {}.", *pReceived);
    }

    ByteBuffer buffer(pReceived->GetPayload());
    ShakeMessage shake = ShakeMessage::Deserialize(buffer);

    m_Version = (std::min)(P2P::PROTOCOL_VERSION, shake.GetVersion());
    m_Capabilities = shake.GetCapabilities();
    m_UserAgent = shake.GetUserAgent();
    m_TotalDifficulty = shake.GetTotalDifficulty();

    return shake;
}

void HandShake::UpdateConnectedPeer(ConnectedPeer& connectedPeer)
{
    connectedPeer.GetPeer()->UpdateLastContactTime();
    connectedPeer.UpdateVersion(m_Version);
    connectedPeer.UpdateCapabilities(m_Capabilities);
    connectedPeer.UpdateUserAgent(m_UserAgent);
    connectedPeer.UpdateTotals(m_TotalDifficulty, 0);
}

std::unique_ptr<RawMessage> HandShake::RetrieveMessage()
{
    std::vector<uint8_t> headerBuffer = m_pSocket->ReceiveSync(P2P::HEADER_LENGTH, true);
    
    if (headerBuffer.size() != P2P::HEADER_LENGTH) { return nullptr; }

    MessageHeader messageHeader = ByteBuffer(std::move(headerBuffer)).Read<MessageHeader>();

    std::vector<uint8_t> payload = m_pSocket->ReceiveSync(messageHeader.GetMessageLength(), false);
    if (payload.size() != messageHeader.GetMessageLength())
    {
        throw PROTOCOL_EXCEPTION("Expected payload not received");
    }

    return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}