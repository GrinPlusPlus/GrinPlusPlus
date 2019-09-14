#pragma once

#include "Message.h"

#include <Core/Models/CompactBlock.h>

class CompactBlockMessage : public IMessage
{
public:
	//
	// Constructors
	//
	CompactBlockMessage(CompactBlock&& block)
		: m_block(std::move(block))
	{

	}
	CompactBlockMessage(const CompactBlock& block)
		: m_block(block)
	{

	}
	CompactBlockMessage(const CompactBlockMessage& other) = default;
	CompactBlockMessage(CompactBlockMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~CompactBlockMessage() = default;

	//
	// Operators
	//
	CompactBlockMessage& operator=(const CompactBlockMessage& other) = default;
	CompactBlockMessage& operator=(CompactBlockMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual CompactBlockMessage* Clone() const override final { return new CompactBlockMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::CompactBlockMsg; }
	inline const CompactBlock& GetCompactBlock() const { return m_block; }

	//
	// Deserialization
	//
	static CompactBlockMessage Deserialize(ByteBuffer& byteBuffer)
	{
		CompactBlock block = CompactBlock::Deserialize(byteBuffer);
		return CompactBlockMessage(std::move(block));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_block.Serialize(serializer);
	}

private:
	CompactBlock m_block;
};