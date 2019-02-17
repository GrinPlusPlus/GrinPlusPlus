#pragma once

#include "Message.h"

#include <Core/Models/Transaction.h>

class TransactionMessage : public IMessage
{
public:
	//
	// Constructors
	//
	TransactionMessage(Transaction&& transaction)
		: m_transaction(std::move(transaction))
	{

	}
	TransactionMessage(const Transaction& transaction)
		: m_transaction(transaction)
	{

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
	virtual TransactionMessage* Clone() const override final { return new TransactionMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::TransactionMsg; }
	inline const Transaction& GetTransaction() const { return m_transaction; }

	//
	// Deserialization
	//
	static TransactionMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Transaction transaction = Transaction::Deserialize(byteBuffer);
		return TransactionMessage(std::move(transaction));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_transaction.Serialize(serializer);
	}

private:
	Transaction m_transaction;
};