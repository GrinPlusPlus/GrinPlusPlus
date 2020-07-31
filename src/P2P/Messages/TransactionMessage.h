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
	virtual IMessagePtr Clone() const override final { return IMessagePtr(new TransactionMessage(*this)); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::TransactionMsg; }
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
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		m_pTransaction->Serialize(serializer);
		LOG_INFO_F("Serialized TransactionMsg: {}", HexUtil::ConvertToHex(serializer.GetBytes()));
	}

private:
	TransactionPtr m_pTransaction;
};