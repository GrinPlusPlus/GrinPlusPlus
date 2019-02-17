#pragma once

#include "Message.h"

#include <Core/Models/Transaction.h>

class StemTransactionMessage : public IMessage
{
public:
	//
	// Constructors
	//
	StemTransactionMessage(Transaction&& transaction)
		: m_transaction(std::move(transaction))
	{

	}
	StemTransactionMessage(const Transaction& transaction)
		: m_transaction(transaction)
	{

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
	virtual StemTransactionMessage* Clone() const override final { return new StemTransactionMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::StemTransaction; }
	inline const Transaction& GetTransaction() const { return m_transaction; }

	//
	// Deserialization
	//
	static StemTransactionMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Transaction transaction = Transaction::Deserialize(byteBuffer);
		return StemTransactionMessage(std::move(transaction));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_transaction.Serialize(serializer);
	}

private:
	Transaction m_transaction;
};