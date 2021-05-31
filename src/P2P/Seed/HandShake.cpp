#include "HandShake.h"

#include "../Messages/HandMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/BanReasonMessage.h"
#include "../ConnectionManager.h"

#include <Core/Exceptions/ProtocolException.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>

static const uint64_t SELF_NONCE = CSPRNG::GenerateRandom(0, UINT64_MAX);

void HandShake::PerformHandshake(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const
{
    EDirection direction = connectedPeer.GetDirection();
    LOG_TRACE_F(
        "Performing handshake with {}({})",
        pSocket,
        (direction == EDirection::INBOUND ? "inbound" : "outbound")
    );

    if (direction == EDirection::OUTBOUND) {
        PerformOutboundHandshake(pSocket, connectedPeer);
    } else {
        PerformInboundHandshake(pSocket, connectedPeer);
    }
}

void HandShake::PerformOutboundHandshake(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const
{
    // Send Hand Message
    TransmitHandMessage(pSocket);

    // Get Shake Message
    auto pReceived = RetrieveMessage(pSocket, *connectedPeer.GetPeer());
    if (pReceived == nullptr) {
        throw PROTOCOL_EXCEPTION("Shake message not received");
    }

    if (pReceived->GetMessageType() != MessageTypes::Shake) {
        throw PROTOCOL_EXCEPTION_F("Expected shake but received {}.", *pReceived);
    }

    ByteBuffer buffer(pReceived->GetPayload());
    ShakeMessage shakeMessage = ShakeMessage::Deserialize(buffer);

    uint32_t version = (std::min)(P2P::PROTOCOL_VERSION, shakeMessage.GetVersion());
    connectedPeer.UpdateVersion(version);
    connectedPeer.UpdateCapabilities(shakeMessage.GetCapabilities());
    connectedPeer.UpdateUserAgent(shakeMessage.GetUserAgent());
    connectedPeer.UpdateTotals(shakeMessage.GetTotalDifficulty(), 0);
}

void HandShake::PerformInboundHandshake(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const
{
    // Get Hand Message
    auto pReceived = RetrieveMessage(pSocket, *connectedPeer.GetPeer());
    if (pReceived == nullptr) {
        throw PROTOCOL_EXCEPTION("Hand message not received.");
    }

    if (pReceived->GetMessageType() != MessageTypes::Hand) {
        throw PROTOCOL_EXCEPTION_F("Expected hand but received {}", *pReceived);
    }

    ByteBuffer byteBuffer(pReceived->GetPayload());
    HandMessage hand_message = HandMessage::Deserialize(byteBuffer);
    
    if (hand_message.GetNonce() == SELF_NONCE) {
        throw PROTOCOL_EXCEPTION("Connected to self");
    }

    if (m_connectionManager.IsConnected(connectedPeer.GetIPAddress())) {
        throw PROTOCOL_EXCEPTION("Already connected to peer");
    }

    connectedPeer.UpdateCapabilities(hand_message.GetCapabilities());
    connectedPeer.UpdateUserAgent(hand_message.GetUserAgent());
    connectedPeer.UpdateTotals(hand_message.GetTotalDifficulty(), 0);

    uint32_t version = (std::min)(P2P::PROTOCOL_VERSION, hand_message.GetVersion());
    connectedPeer.UpdateVersion(version);

    // Send Shake Message
    TransmitShakeMessage(pSocket, version);
}

void HandShake::TransmitHandMessage(const Socket::Ptr& pSocket) const
{
    IPAddress localHostIP = IPAddress::CreateV4({ 0x7F, 0x00, 0x00, 0x01 });
    HandMessage hand(
        P2P::PROTOCOL_VERSION,
        Capabilities::FAST_SYNC_NODE,
        SELF_NONCE,
        Global::GetGenesisHash(),
        m_pSyncStatus->GetBlockDifficulty(),
        SocketAddress(localHostIP, Global::GetConfig().GetP2PPort()),
        SocketAddress(localHostIP, pSocket->GetPort()),
        P2P::USER_AGENT
    );

    if (!pSocket->SendSync(hand.Serialize(ProtocolVersion::Local()), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send hand message.");
    }
}

void HandShake::TransmitShakeMessage(const Socket::Ptr& pSocket, const uint32_t protocolVersion) const
{
    ShakeMessage shakeMessage(
        protocolVersion,
        Capabilities::FAST_SYNC_NODE,
        Global::GetGenesisHash(),
        m_pSyncStatus->GetBlockDifficulty(),
        P2P::USER_AGENT
    );

    if (!pSocket->SendSync(shakeMessage.Serialize(ProtocolVersion::ToEnum(protocolVersion)), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send shake message.");
    }
}

std::unique_ptr<RawMessage> HandShake::RetrieveMessage(const Socket::Ptr& pSocket, const Peer& peer) const
{
    std::vector<uint8_t> headerBuffer = pSocket->ReceiveSync(11, true);
    if (headerBuffer.size() != 11) {
        return nullptr;
    }

    MessageHeader messageHeader = ByteBuffer(std::move(headerBuffer)).Read<MessageHeader>();
    LOG_TRACE_F("Received '{}' message from {}", messageHeader, peer);

    std::vector<uint8_t> payload = pSocket->ReceiveSync(messageHeader.GetMessageLength(), false);
    if (payload.size() != messageHeader.GetMessageLength()) {
        throw PROTOCOL_EXCEPTION("Expected payload not received");
    }

    peer.UpdateLastContactTime();
    return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}