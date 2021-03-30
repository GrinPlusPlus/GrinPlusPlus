#pragma once

#include "Message.h"

#include <Crypto/Models/Hash.h>

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
	IMessagePtr Clone() const final { return IMessagePtr(new GetTransactionMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::GetTransactionMsg; }
	const Hash& GetKernelHash() const { return m_kernelHash; }

	//
	// Deserialization
	//
	static GetTransactionMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Hash kernelHash = byteBuffer.ReadBigInteger<32>();
		return GetTransactionMessage(std::move(kernelHash));
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.AppendBigInteger<32>(m_kernelHash);
	}

private:
	Hash m_kernelHash;
};