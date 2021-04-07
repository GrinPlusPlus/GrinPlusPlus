#pragma once

#include "Message.h"
#include <Crypto/Models/Hash.h>

// Second part of a handshake, receiver of the first part replies with its own version and characteristics.
class TxHashSetArchiveMessage : public IMessage
{
public:
	//
	// Constructors
	//
	TxHashSetArchiveMessage(Hash blockHash, const uint64_t blockHeight, const uint64_t zippedDataSize)
		: m_blockHash(std::move(blockHash)), m_blockHeight(blockHeight), m_zippedDataSize(zippedDataSize)
	{

	}
	TxHashSetArchiveMessage(const TxHashSetArchiveMessage& other) = default;
	TxHashSetArchiveMessage(TxHashSetArchiveMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~TxHashSetArchiveMessage() = default;

	//
	// Operators
	//
	TxHashSetArchiveMessage& operator=(const TxHashSetArchiveMessage& other) = default;
	TxHashSetArchiveMessage& operator=(TxHashSetArchiveMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new TxHashSetArchiveMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::TxHashSetArchive; }
	const Hash& GetBlockHash() const { return m_blockHash; }
	uint64_t GetBlockHeight() const { return m_blockHeight; }
	uint64_t GetZippedSize() const { return m_zippedDataSize; }

	//
	// Deserialization
	//
	static TxHashSetArchiveMessage Deserialize(ByteBuffer& byteBuffer)
	{
		Hash blockHash = byteBuffer.ReadBigInteger<32>();
		const uint64_t blockHeight = byteBuffer.ReadU64();
		const uint64_t zippedDataSize = byteBuffer.ReadU64();

		return TxHashSetArchiveMessage(std::move(blockHash), blockHeight, zippedDataSize);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.AppendBigInteger(m_blockHash);
		serializer.Append<uint64_t>(m_blockHeight);
		serializer.Append<uint64_t>(m_zippedDataSize);
	}

private:
	Hash m_blockHash;
	uint64_t m_blockHeight;
	uint64_t m_zippedDataSize;
};