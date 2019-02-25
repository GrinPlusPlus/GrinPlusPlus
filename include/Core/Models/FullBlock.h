#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <vector>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/TransactionBody.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class FullBlock
{
public:
	//
	// Constructors
	//
	FullBlock(BlockHeader&& blockHeader, TransactionBody&& transactionBody);
	FullBlock(const FullBlock& other) = default;
	FullBlock(FullBlock&& other) noexcept = default;
	FullBlock() = default;

	//
	// Destructor
	//
	~FullBlock() = default;

	//
	// Operators
	//
	FullBlock& operator=(const FullBlock& other) = default;
	FullBlock& operator=(FullBlock&& other) noexcept = default;

	//
	// Getters
	//
	inline const BlockHeader& GetBlockHeader() const { return m_blockHeader; }
	inline const TransactionBody& GetTransactionBody() const { return m_transactionBody; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static FullBlock Deserialize(ByteBuffer& byteBuffer);

	//
	// Hashing
	//
	inline const Hash& GetHash() const { return m_blockHeader.GetHash(); }

	//
	// Validation Status
	//
	inline bool WasValidated() const { return m_validated; }
	inline void MarkAsValidated() const { m_validated = true; }

private:
	BlockHeader m_blockHeader;
	TransactionBody m_transactionBody;
	mutable bool m_validated;
};