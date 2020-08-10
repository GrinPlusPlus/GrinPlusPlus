#pragma once

#include "Message.h"

#include <Core/Models/FullBlock.h>

class BlockMessage : public IMessage
{
public:
	//
	// Constructors
	//
	BlockMessage(FullBlock&& block)
		: m_block(std::move(block))
	{

	}
	BlockMessage(const BlockMessage& other) = default;
	BlockMessage(BlockMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~BlockMessage() = default;

	//
	// Operators
	//
	BlockMessage& operator=(const BlockMessage& other) = default;
	BlockMessage& operator=(BlockMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new BlockMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::Block; }
	const FullBlock& GetBlock() const { return m_block; }

	//
	// Deserialization
	//
	static BlockMessage Deserialize(ByteBuffer& byteBuffer)
	{
		FullBlock block = FullBlock::Deserialize(byteBuffer);
		return BlockMessage(std::move(block));
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		m_block.Serialize(serializer);
	}

private:
	FullBlock m_block;
};