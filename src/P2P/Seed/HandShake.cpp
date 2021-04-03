#include "HandShake.h"

#include "../Messages/HandMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/BanReasonMessage.h"
#include "../ConnectionManager.h"

#include <Core/Exceptions/ProtocolException.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>
#include <Common/Util/VectorUtil.h>

static const uint64_t SELF_NONCE = CSPRNG::GenerateRandom(0, UINT64_MAX);

void HandShake::PerformHandshake(Socket& socket, ConnectedPeer& connectedPeer) const
{
    EDirection direction = connectedPeer.GetDirection();
    LOG_TRACE_F(
        "Performing handshake with {}({})",
        socket,
        (direction == EDirection::INBOUND ? "inbound" : "outbound")
    );

    if (direction == EDirection::OUTBOUND) {
        PerformOutboundHandshake(socket, connectedPeer);
    } else {
        PerformInboundHandshake(socket, connectedPeer);
    }
}

void HandShake::PerformOutboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const
{
    // Send Hand Message
    TransmitHandMessage(socket);

    // Get Shake Message
    auto pReceived = RetrieveMessage(socket, *connectedPeer.GetPeer());
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

void HandShake::PerformInboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const
{
    // Get Hand Message
    auto pReceived = RetrieveMessage(socket, *connectedPeer.GetPeer());
    if (pReceived == nullptr) {
        throw PROTOCOL_EXCEPTION("No message received from incoming peer.");
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
    TransmitShakeMessage(socket, version);
}

void HandShake::TransmitHandMessage(Socket& socket) const
{
    IPAddress localHostIP = IPAddress::CreateV4({ 0x7F, 0x00, 0x00, 0x01 });
    HandMessage hand(
        P2P::PROTOCOL_VERSION,
        Capabilities::FAST_SYNC_NODE,
        SELF_NONCE,
        Global::GetGenesisHash(),
        m_pSyncStatus->GetBlockDifficulty(),
        SocketAddress(localHostIP, Global::GetConfig().GetP2PPort()),
        SocketAddress(localHostIP, socket.GetPort()),
        P2P::USER_AGENT
    );

    if (!socket.Send(hand.Serialize(ProtocolVersion::Local()), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send hand message.");
    }
}

void HandShake::TransmitShakeMessage(Socket& socket, const uint32_t protocolVersion) const
{
    ShakeMessage shakeMessage(
        protocolVersion,
        Capabilities::FAST_SYNC_NODE,
        Global::GetGenesisHash(),
        m_pSyncStatus->GetBlockDifficulty(),
        P2P::USER_AGENT
    );

    if (!socket.Send(shakeMessage.Serialize(ProtocolVersion::ToEnum(protocolVersion)), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send shake message.");
    }
}

std::unique_ptr<RawMessage> HandShake::RetrieveMessage(Socket& socket, const Peer& peer) const
{
    std::vector<uint8_t> headerBuffer;
    const bool received = socket.Receive(11, true, Socket::BLOCKING, headerBuffer);
    if (!received) {
        return nullptr;
    }

    MessageHeader messageHeader = ByteBuffer(std::move(headerBuffer)).Read<MessageHeader>();
    LOG_TRACE_F("Received '{}' message from {}", messageHeader, peer);

    std::vector<uint8_t> payload;
    const bool bPayloadRetrieved = socket.Receive(
        messageHeader.GetMessageLength(),
        false,
        Socket::BLOCKING,
        payload
    );
    if (!bPayloadRetrieved) {
        throw PROTOCOL_EXCEPTION("Expected payload not received");
    }

    peer.UpdateLastContactTime();
    return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}