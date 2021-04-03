#pragma once

#include "Message.h"

#include <Core/Models/Transaction.h>
#include <cassert>

class TransactionMessage : public IMessage
{
public:
	//
	// Constructors
	//
	TransactionMessage(TransactionPtr pTransaction)
		: m_pTransaction(pTransaction)
	{
		assert(pTransaction != nullptr);
	}
	TransactionMessage(const TransactionMessage& other) = default;
	TransactionMessage(TransactionMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~TransactionMessage() = default;

	//
	// Operators
	//
	TransactionMessage& operator=(const TransactionMessage& other) = default;
	TransactionMessage& operator=(TransactionMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new TransactionMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::TransactionMsg; }
	TransactionPtr GetTransaction() const { return m_pTransaction; }

	//
	// Deserialization
	//
	static TransactionMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Transaction transaction = Transaction::Deserialize(byteBuffer);
		return TransactionMessage(std::make_shared<Transaction>(transaction));
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		m_pTransaction->Serialize(serializer);
	}

private:
	TransactionPtr m_pTransaction;
};