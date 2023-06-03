#include "CapabilitiesExchanger.h"

#include "PeerManager.h"

#include "../Messages/GetPeerAddressesMessage.h"
#include "../Messages/PeerAddressesMessage.h"
#include "../ConnectionManager.h"

#include <Core/Exceptions/ProtocolException.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>
#include <Core/Traits/Lockable.h>


CapabilitiesExchanger::CapabilitiesExchanger(
    Locked<PeerManager> peerManager)
    : m_peerManager(peerManager) 
{

}

void CapabilitiesExchanger::Perform(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer)
{
    EDirection direction = connectedPeer.GetDirection();
    
    if (direction == EDirection::OUTBOUND)
    {
        LOG_TRACE_F("Sending capabilities to: {}", pSocket);
        TransmitGetPeerAddressesMessage(pSocket);  // Send Capabilities
        const PeerAddressesMessage peerAddressesMessage = ReceivePeerAddrsCapabilities(pSocket);
        
        connectedPeer.GetPeer()->UpdateLastContactTime();

        const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();
        LOG_TRACE_F("Received {} addresses from {}.", peerAddresses.size(), connectedPeer.GetIPAddress());

        m_peerManager.Write()->AddFreshPeers(peerAddresses);
    }
    else if (direction == EDirection::INBOUND)
    {
        LOG_TRACE_F("Reading capabilities to: {}", pSocket);
        const GetPeerAddressesMessage getPeerAddressesMessage = ReceiveGetPeerAddressesMessage(pSocket);  // Capabilities are inside here
        const Capabilities capabilities = getPeerAddressesMessage.GetCapabilities();

        const std::vector<PeerPtr> peers = m_peerManager.Read()->GetPeers(capabilities.GetCapability(), P2P::MAX_PEER_ADDRS);
        std::vector<SocketAddress> socketAddresses;
        std::transform(
            peers.cbegin(),
            peers.cend(),
            std::back_inserter(socketAddresses),
            [this](const PeerPtr& peer) { return SocketAddress(peer->GetIPAddress(), Global::GetConfig().GetP2PPort()); }
        );

        LOG_TRACE_F("Sending {} addresses to {}.", socketAddresses.size(), connectedPeer.GetIPAddress());

        TransmitPeerAddressesMessage(pSocket, PeerAddressesMessage{ std::move(socketAddresses) });
    }
}

const PeerAddressesMessage CapabilitiesExchanger::ReceivePeerAddrsCapabilities(const Socket::Ptr& pSocket)
{
    auto pReceived = RetrieveMessage(pSocket); // Get PeerAddrs Message
    if (pReceived == nullptr) {
        throw PROTOCOL_EXCEPTION_F("PeerAddrs message not received from {}", pSocket->GetIPAddress());
    }

    if (pReceived->GetMessageType() != MessageTypes::PeerAddrs) {
        throw PROTOCOL_EXCEPTION_F("Expected PeerAddrs from {} but received {}.", pSocket->GetIPAddress() , *pReceived);
    }

    ByteBuffer buffer(pReceived->GetPayload());

    return PeerAddressesMessage::Deserialize(buffer);
}

void CapabilitiesExchanger::TransmitGetPeerAddressesMessage(const Socket::Ptr& pSocket)
{
    GetPeerAddressesMessage capabilities(Capabilities::ECapability::FAST_SYNC_NODE);
        
    if (!pSocket->SendSync(capabilities.Serialize(ProtocolVersion::Local()), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send hcapabilities");
    }
}

const GetPeerAddressesMessage CapabilitiesExchanger::ReceiveGetPeerAddressesMessage(const Socket::Ptr& pSocket)
{
    auto pReceived = RetrieveMessage(pSocket); // Get GetPeerAddrs Message
    if (pReceived == nullptr) {
        throw PROTOCOL_EXCEPTION_F("PeerAddrs message not received from {}", pSocket->GetIPAddress());
    }

    if (pReceived->GetMessageType() != MessageTypes::GetPeerAddrs) {
        throw PROTOCOL_EXCEPTION_F("Expected PeerAddrs from {} but received {}.", pSocket->GetIPAddress(), *pReceived);
    }

    ByteBuffer buffer(pReceived->GetPayload());

    return GetPeerAddressesMessage::Deserialize(buffer);
}

void CapabilitiesExchanger::TransmitPeerAddressesMessage(const Socket::Ptr& pSocket, const PeerAddressesMessage addresses)
{
    if (!pSocket->SendSync(addresses.Serialize(ProtocolVersion::Local()), true)) {
        throw PROTOCOL_EXCEPTION("Failed to send hcapabilities");
    }
}

std::unique_ptr<RawMessage> CapabilitiesExchanger::RetrieveMessage(const Socket::Ptr& pSocket) 
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
