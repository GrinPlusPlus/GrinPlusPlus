#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/TransactionBody.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Printable.h>

class FullBlock : public Traits::IPrintable
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
	const BlockHeader& GetBlockHeader() const { return m_blockHeader; }
	const TransactionBody& GetTransactionBody() const { return m_transactionBody; }
	const std::vector<TransactionInput>& GetInputs() const { return m_transactionBody.GetInputs(); }
	const std::vector<TransactionOutput>& GetOutputs() const { return m_transactionBody.GetOutputs(); }
	const std::vector<TransactionKernel>& GetKernels() const { return m_transactionBody.GetKernels(); }
	uint64_t GetHeight() const { return m_blockHeader.GetHeight(); }
	const Hash& GetPreviousHash() const { return m_blockHeader.GetPreviousBlockHash(); }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static FullBlock Deserialize(ByteBuffer& byteBuffer);

	//
	// Hashing
	//
	const Hash& GetHash() const { return m_blockHeader.GetHash(); }

	//
	// Validation Status
	//
	bool WasValidated() const { return m_validated; }
	void MarkAsValidated() const { m_validated = true; }

	//
	// Traits
	//
	virtual std::string Format() const override final { return GetHash().ToHex(); }

private:
	BlockHeader m_blockHeader;
	TransactionBody m_transactionBody;
	mutable bool m_validated;
};