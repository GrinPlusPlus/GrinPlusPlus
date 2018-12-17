#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <vector>
#include <Core/BlockHeader.h>
#include <Core/TransactionBody.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

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
	inline const Hash& Hash() const { return m_blockHeader.Hash(); }

private:
	BlockHeader m_blockHeader;
	TransactionBody m_transactionBody;
};