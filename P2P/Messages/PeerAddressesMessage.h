#pragma once

#include "Message.h"

#include <P2P/SocketAddress.h>
#include <vector>

class PeerAddressesMessage : public IMessage
{
public:
	//
	// Constructors
	//
	PeerAddressesMessage(std::vector<SocketAddress>&& peerAddresses)
		: m_peerAddresses(std::move(peerAddresses))
	{

	}
	PeerAddressesMessage(const PeerAddressesMessage& other) = default;
	PeerAddressesMessage(PeerAddressesMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~PeerAddressesMessage() = default;

	//
	// Operators
	//
	PeerAddressesMessage& operator=(const PeerAddressesMessage& other) = default;
	PeerAddressesMessage& operator=(PeerAddressesMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual PeerAddressesMessage* Clone() const override final { return new PeerAddressesMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::PeerAddrs; }
	inline const std::vector<SocketAddress>& GetPeerAddresses() const { return m_peerAddresses; }

	//
	// Deserialization
	//
	static PeerAddressesMessage Deserialize(ByteBuffer& byteBuffer)
	{
		std::vector<SocketAddress> peerAddresses;

		const uint32_t numAddresses = byteBuffer.ReadU32();
		for (uint32_t i = 0; i < numAddresses; i++)
		{
			peerAddresses.emplace_back(SocketAddress::Deserialize(byteBuffer));
		}

		return PeerAddressesMessage(std::move(peerAddresses));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.Append<uint32_t>((uint32_t)m_peerAddresses.size());
		for (auto iter = m_peerAddresses.cbegin(); iter != m_peerAddresses.cend(); iter++)
		{
			iter->Serialize(serializer);
		}
	}

private:
	std::vector<SocketAddress> m_peerAddresses;
};