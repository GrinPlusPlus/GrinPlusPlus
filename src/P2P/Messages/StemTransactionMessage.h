#pragma once

#include "Message.h"

#include <Core/Models/Transaction.h>
#include <cassert>

class StemTransactionMessage : public IMessage
{
public:
	//
	// Constructors
	//
	StemTransactionMessage(TransactionPtr pTransaction)
		: m_pTransaction(pTransaction)
	{
		assert(pTransaction != nullptr);
	}
	StemTransactionMessage(const StemTransactionMessage& other) = default;
	StemTransactionMessage(StemTransactionMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~StemTransactionMessage() = default;

	//
	// Operators
	//
	StemTransactionMessage& operator=(const StemTransactionMessage& other) = default;
	StemTransactionMessage& operator=(StemTransactionMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual IMessagePtr Clone() const override final { return IMessagePtr(new StemTransactionMessage(*this)); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::StemTransaction; }
	TransactionPtr GetTransaction() const { return m_pTransaction; }

	//
	// Deserialization
	//
	static StemTransactionMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Transaction transaction = Transaction::Deserialize(byteBuffer);
		return StemTransactionMessage(std::make_shared<Transaction>(transaction));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_pTransaction->Serialize(serializer);
	}

private:
	TransactionPtr m_pTransaction;
};