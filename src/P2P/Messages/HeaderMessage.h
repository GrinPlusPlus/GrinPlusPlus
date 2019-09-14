#pragma once

#include "Message.h"

#include <Core/Models/BlockHeader.h>

class HeaderMessage : public IMessage
{
public:
	//
	// Constructors
	//
	HeaderMessage(BlockHeader&& header)
		: m_header(std::move(header))
	{

	}
	HeaderMessage(const BlockHeader& header)
		: m_header(std::move(header))
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
	virtual HeaderMessage* Clone() const override final { return new HeaderMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::Header; }
	inline const BlockHeader& GetHeader() const { return m_header; }

	//
	// Deserialization
	//
	static HeaderMessage Deserialize(ByteBuffer& byteBuffer)
	{
		BlockHeader header = BlockHeader::Deserialize(byteBuffer);
		return HeaderMessage(std::move(header));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_header.Serialize(serializer);
	}

private:
	BlockHeader m_header;
};