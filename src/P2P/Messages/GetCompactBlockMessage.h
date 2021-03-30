#pragma once

#include "Message.h"

#include <Crypto/Models/BigInteger.h>
#include <Crypto/Models/Hash.h>

class GetCompactBlockMessage : public IMessage
{
public:
	//
	// Constructors
	//
	GetCompactBlockMessage(const Hash& hash)
		: m_hash(hash)
	{

	}
	GetCompactBlockMessage(Hash&& hash)
		: m_hash(std::move(hash))
	{

	}
	GetCompactBlockMessage(const GetCompactBlockMessage& other) = default;
	GetCompactBlockMessage(GetCompactBlockMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~GetCompactBlockMessage() = default;

	//
	// Operators
	//
	GetCompactBlockMessage& operator=(const GetCompactBlockMessage& other) = default;
	GetCompactBlockMessage& operator=(GetCompactBlockMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new GetCompactBlockMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::GetCompactBlock; }
	const Hash& GetHash() const { return m_hash; }

	//
	// Deserialization
	//
	static GetCompactBlockMessage Deserialize(ByteBuffer& byteBuffer)
	{
		return GetCompactBlockMessage(byteBuffer.ReadBigInteger<32>());
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.AppendBigInteger<32>(m_hash);
	}

private:
	Hash m_hash;
};