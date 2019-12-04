#pragma once

#include "Message.h"

#include <Core/Models/BlockHeader.h>

class HeaderMessage : public IMessage
{
public:
	//
	// Constructors
	//
	HeaderMessage(BlockHeaderPtr pHeader)
		: m_pHeader(pHeader)
	{

	}
	HeaderMessage(const HeaderMessage& other) = default;
	HeaderMessage(HeaderMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~HeaderMessage() = default;

	//
	// Operators
	//
	HeaderMessage& operator=(const HeaderMessage& other) = default;
	HeaderMessage& operator=(HeaderMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual IMessagePtr Clone() const override final { return IMessagePtr(new HeaderMessage(*this)); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::Header; }
	const BlockHeaderPtr& GetHeader() const { return m_pHeader; }

	//
	// Deserialization
	//
	static HeaderMessage Deserialize(ByteBuffer& byteBuffer)
	{
		BlockHeaderPtr pHeader = std::make_shared<const BlockHeader>(BlockHeader::Deserialize(byteBuffer));
		return HeaderMessage(pHeader);
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_pHeader->Serialize(serializer);
	}

private:
	BlockHeaderPtr m_pHeader;
};