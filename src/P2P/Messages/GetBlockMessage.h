#pragma once

#include "Message.h"

#include <Crypto/BigInteger.h>
#include <Crypto/Hash.h>

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
	virtual GetBlockMessage* Clone() const override final { return new GetBlockMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::GetBlock; }
	inline const Hash& GetHash() const { return m_hash; }

	//
	// Deserialization
	//
	static GetBlockMessage Deserialize(ByteBuffer& byteBuffer)
	{
		return GetBlockMessage(byteBuffer.ReadBigInteger<32>());
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.AppendBigInteger<32>(m_hash);
	}

private:
	Hash m_hash;
};