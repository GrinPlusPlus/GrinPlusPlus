#pragma once

#include <Net/Socket.h>
#include <P2P/SyncStatus.h>
#include <P2P/ConnectedPeer.h>
#include <Core/Traits/Lockable.h>

#include "PeerManager.h"

#include "../Messages/RawMessage.h"
#include "../Messages/PeerAddressesMessage.h"
#include "../Messages/GetPeerAddressesMessage.h"

#include <memory>

class PeerManager;

class CapabilitiesExchanger
{
public:
	CapabilitiesExchanger(Locked<PeerManager> peerManager);

	void Perform(const Socket::Ptr& pSocket, ConnectedPeer& connectedPeer);

private:
	const PeerAddressesMessage ReceivePeerAddrsCapabilities(const Socket::Ptr& pSocket);
	void TransmitGetPeerAddressesMessage(const Socket::Ptr& pSocket);
	const GetPeerAddressesMessage ReceiveGetPeerAddressesMessage(const Socket::Ptr& pSocket);
	void TransmitPeerAddressesMessage(const Socket::Ptr& pSocket, const PeerAddressesMessage addresses);

	std::unique_ptr<RawMessage> RetrieveMessage(const Socket::Ptr& pSocket) ;

	Locked<PeerManager> m_peerManager;
};