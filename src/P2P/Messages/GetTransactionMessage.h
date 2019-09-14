#pragma once

#include "Message.h"

#include <Crypto/Hash.h>

class GetTransactionMessage : public IMessage
{
public:
	//
	// Constructors
	//
	GetTransactionMessage(Hash&& kernelHash)
		: m_kernelHash(std::move(kernelHash))
	{

	}
	GetTransactionMessage(const Hash& kernelHash)
		: m_kernelHash(kernelHash)
	{

	}
	GetTransactionMessage(const GetTransactionMessage& other) = default;
	GetTransactionMessage(GetTransactionMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~GetTransactionMessage() = default;

	//
	// Operators
	//
	GetTransactionMessage& operator=(const GetTransactionMessage& other) = default;
	GetTransactionMessage& operator=(GetTransactionMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual GetTransactionMessage* Clone() const override final { return new GetTransactionMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::GetTransactionMsg; }
	inline const Hash& GetKernelHash() const { return m_kernelHash; }

	//
	// Deserialization
	//
	static GetTransactionMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Hash kernelHash = byteBuffer.ReadBigInteger<32>();
		return GetTransactionMessage(std::move(kernelHash));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.AppendBigInteger<32>(m_kernelHash);
	}

private:
	Hash m_kernelHash;
};