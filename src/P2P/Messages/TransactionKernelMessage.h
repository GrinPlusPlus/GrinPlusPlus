#pragma once

#include "Message.h"

#include <Crypto/Models/Hash.h>

class TransactionKernelMessage : public IMessage
{
public:
	//
	// Constructors
	//
	TransactionKernelMessage(Hash&& kernelHash)
		: m_kernelHash(std::move(kernelHash))
	{

	}
	TransactionKernelMessage(const Hash& kernelHash)
		: m_kernelHash(kernelHash)
	{

	}
	TransactionKernelMessage(const TransactionKernelMessage& other) = default;
	TransactionKernelMessage(TransactionKernelMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~TransactionKernelMessage() = default;

	//
	// Operators
	//
	TransactionKernelMessage& operator=(const TransactionKernelMessage& other) = default;
	TransactionKernelMessage& operator=(TransactionKernelMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new TransactionKernelMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::TransactionKernelMsg; }
	const Hash& GetKernelHash() const { return m_kernelHash; }

	//
	// Deserialization
	//
	static TransactionKernelMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Hash kernelHash = byteBuffer.ReadBigInteger<32>();
		return TransactionKernelMessage(std::move(kernelHash));
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.AppendBigInteger<32>(m_kernelHash);
	}

private:
	Hash m_kernelHash;
};