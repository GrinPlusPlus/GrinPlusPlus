#pragma once

#include "Message.h"
#include <Crypto/Hash.h>

// Second part of a handshake, receiver of the first part replies with its own version and characteristics.
class TxHashSetArchiveMessage : public IMessage
{
public:
	//
	// Constructors
	//
	TxHashSetArchiveMessage(Hash&& blockHash, const uint64_t blockHeight, const uint64_t zippedDataSize)
		: m_blockHash(std::move(blockHash)), m_blockHeight(blockHeight), m_zippedDataSize(std::move(zippedDataSize))
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
	virtual TxHashSetArchiveMessage* Clone() const override final { return new TxHashSetArchiveMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::TxHashSetArchive; }
	inline const Hash& GetBlockHash() const { return m_blockHash; }
	inline uint64_t GetBlockHeight() const { return m_blockHeight; }
	inline uint64_t GetZippedSize() const { return m_zippedDataSize; }

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
	virtual void SerializeBody(Serializer& serializer) const override final
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