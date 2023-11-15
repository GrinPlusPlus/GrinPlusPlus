#include "PingPong.h"

#include "../Messages/PingMessage.h"
#include "../Messages/PongMessage.h"
#include "../ConnectionManager.h"

#include <Core/Exceptions/ProtocolException.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>
#include <Core/Traits/Lockable.h>


const PongMessage PingPong::Execute(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer) const
{
    const PongMessage pongMessage = SendPing(pSocket);
    connectedPeer.GetPeer()->UpdateLastContactTime();
    return pongMessage;
}

const PongMessage PingPong::SendPing(const Socket::Ptr& pSocket) const
{
    LOG_TRACE_F("Sending Ping to: {}", pSocket);
    TransmitPingMessage(pSocket);  // Send Ping
    
    auto pReceived = RetrieveMessage(pSocket); // Get Pong Message
    if (pReceived == nullptr) {
        throw PROTOCOL_EXCEPTION_F("Pong message not received from {}", pSocket->GetIPAddress());
    }

    ByteBuffer buffer(pReceived->GetPayload());

    return PongMessage::Deserialize(buffer);
}

void PingPong::TransmitPingMessage(const Socket::Ptr& pSocket) const
{    
    const PingMessage ping = PingMessage(m_pSyncStatus->GetBlockDifficulty(), m_pSyncStatus->GetBlockHeight());
    if (!pSocket->SendSync(ping.Serialize(ProtocolVersion::Local()), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send hcapabilities");
    }
}

std::unique_ptr<RawMessage> PingPong::RetrieveMessage(const Socket::Ptr& pSocket) const
{
    std::vector<uint8_t> headerBuffer = pSocket->ReceiveSync(P2P::HEADER_LENGTH, true);
    
    if (headerBuffer.size() != P2P::HEADER_LENGTH) {
        return nullptr;
    }

    MessageHeader messageHeader = ByteBuffer(std::move(headerBuffer)).Read<MessageHeader>();
    
    std::vector<uint8_t> payload = pSocket->ReceiveSync(messageHeader.GetMessageLength(), false);
    if (payload.size() != messageHeader.GetMessageLength()) {
        throw PROTOCOL_EXCEPTION("Expected payload not received");
    }

    return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}
