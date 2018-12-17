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
	virtual GetPeerAddressesMessage* Clone() const override final { return new GetPeerAddressesMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::GetPeerAddrs; }
	inline const Capabilities& GetCapabilities() const { return m_capabilities; }

	//
	// Deserialization
	//
	static GetPeerAddressesMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const Capabilities capabilities = Capabilities::Deserialize(byteBuffer);
		return GetPeerAddressesMessage(capabilities);
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_capabilities.Serialize(serializer);
	}

private:
	// Filters on the capabilities we'd like the peers to have
	const Capabilities m_capabilities;
};