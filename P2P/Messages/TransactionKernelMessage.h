#pragma once

#include "Message.h"

#include <Crypto/Hash.h>

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
	virtual TransactionKernelMessage* Clone() const override final { return new TransactionKernelMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::TransactionKernelMsg; }
	inline const Hash& GetKernelHash() const { return m_kernelHash; }

	//
	// Deserialization
	//
	static TransactionKernelMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Hash kernelHash = byteBuffer.ReadBigInteger<32>();
		return TransactionKernelMessage(std::move(kernelHash));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.AppendBigInteger<32>(m_kernelHash);
	}

private:
	Hash m_kernelHash;
};