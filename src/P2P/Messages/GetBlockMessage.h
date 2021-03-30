#pragma once

#include "Message.h"

#include <Crypto/Models/BigInteger.h>
#include <Crypto/Models/Hash.h>

class GetBlockMessage : public IMessage
{
public:
	//
	// Constructors
	//
	GetBlockMessage(const Hash& hash)
		: m_hash(hash)
	{

	}
	GetBlockMessage(Hash&& hash)
		: m_hash(std::move(hash))
	{

	}
	GetBlockMessage(const GetBlockMessage& other) = default;
	GetBlockMessage(GetBlockMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~GetBlockMessage() = default;

	//
	// Operators
	//
	GetBlockMessage& operator=(const GetBlockMessage& other) = default;
	GetBlockMessage& operator=(GetBlockMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new GetBlockMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::GetBlock; }
	const Hash& GetHash() const { return m_hash; }

	//
	// Deserialization
	//
	static GetBlockMessage Deserialize(ByteBuffer& byteBuffer)
	{
		return GetBlockMessage(byteBuffer.ReadBigInteger<32>());
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.AppendBigInteger<32>(m_hash);
	}

private:
	Hash m_hash;
};