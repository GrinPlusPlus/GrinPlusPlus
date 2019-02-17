#pragma once

#include "Message.h"

#include <Crypto/BigInteger.h>
#include <Crypto/Hash.h>

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
	virtual GetCompactBlockMessage* Clone() const override final { return new GetCompactBlockMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::GetCompactBlock; }
	inline const Hash& GetHash() const { return m_hash; }

	//
	// Deserialization
	//
	static GetCompactBlockMessage Deserialize(ByteBuffer& byteBuffer)
	{
		return GetCompactBlockMessage(byteBuffer.ReadBigInteger<32>());
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.AppendBigInteger<32>(m_hash);
	}

private:
	Hash m_hash;
};