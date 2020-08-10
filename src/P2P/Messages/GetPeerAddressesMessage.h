#pragma once

#include "Message.h"
#include <P2P/Capabilities.h>

class GetPeerAddressesMessage : public IMessage
{
public:
	//
	// Constructors
	//
	GetPeerAddressesMessage(const Capabilities& capabilities)
		: m_capabilities(capabilities)
	{

	}
	GetPeerAddressesMessage(const GetPeerAddressesMessage& other) = default;
	GetPeerAddressesMessage(GetPeerAddressesMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~GetPeerAddressesMessage() = default;

	//
	// Operators
	//
	GetPeerAddressesMessage& operator=(const GetPeerAddressesMessage& other) = default;
	GetPeerAddressesMessage& operator=(GetPeerAddressesMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new GetPeerAddressesMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::GetPeerAddrs; }
	const Capabilities& GetCapabilities() const { return m_capabilities; }

	//
	// Deserialization
	//
	static GetPeerAddressesMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const Capabilities capabilities = Capabilities::Deserialize(byteBuffer);
		return GetPeerAddressesMessage(capabilities);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		m_capabilities.Serialize(serializer);
	}

private:
	// Filters on the capabilities we'd like the peers to have
	const Capabilities m_capabilities;
};