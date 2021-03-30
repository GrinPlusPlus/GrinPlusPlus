#pragma once

#include "Message.h"
#include <Crypto/Models/Hash.h>

// Second part of a handshake, receiver of the first part replies with its own version and characteristics.
class TxHashSetRequestMessage : public IMessage
{
public:
	//
	// Constructors
	//
	TxHashSetRequestMessage(Hash&& blockHash, const uint64_t blockHeight)
		: m_blockHash(std::move(blockHash)), m_blockHeight(blockHeight)
	{

	}
	TxHashSetRequestMessage(const TxHashSetRequestMessage& other) = default;
	TxHashSetRequestMessage(TxHashSetRequestMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~TxHashSetRequestMessage() = default;

	//
	// Operators
	//
	TxHashSetRequestMessage& operator=(const TxHashSetRequestMessage& other) = default;
	TxHashSetRequestMessage& operator=(TxHashSetRequestMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new TxHashSetRequestMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::TxHashSetRequest; }
	const Hash& GetBlockHash() const { return m_blockHash; }

	//
	// Deserialization
	//
	static TxHashSetRequestMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Hash blockHash = byteBuffer.ReadBigInteger<32>();
		const uint64_t blockHeight = byteBuffer.ReadU64();

		return TxHashSetRequestMessage(std::move(blockHash), blockHeight);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.AppendBigInteger(m_blockHash);
		serializer.Append<uint64_t>(m_blockHeight);
	}

private:
	Hash m_blockHash;
	uint64_t m_blockHeight;
};