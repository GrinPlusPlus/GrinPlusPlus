#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <vector>
#include <Core/BlockHeader.h>
#include <Core/ShortId.h>
#include <Core/TransactionOutput.h>
#include <Core/TransactionKernel.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

class CompactBlock
{
public:
	//
	// Constructors
	//
	CompactBlock(BlockHeader&& blockHeader, const uint64_t nonce, std::vector<TransactionOutput>&& fullOutputs, std::vector<TransactionKernel>&& fullKernels, std::vector<ShortId>&& shortIds);
	CompactBlock(const CompactBlock& other) = default;
	CompactBlock(CompactBlock&& other) noexcept = default;
	CompactBlock() = default;

	//
	// Destructor
	//
	~CompactBlock() = default;

	//
	// Operators
	//
	CompactBlock& operator=(const CompactBlock& other) = default;
	CompactBlock& operator=(CompactBlock&& other) noexcept = default;

	//
	// Getters
	//
	inline const BlockHeader& GetBlockHeader() const { return m_blockHeader; }
	inline uint64_t GetNonce() const { return m_nonce; }
	inline const std::vector<TransactionOutput>& GetOutputs() const { return m_outputs; }
	inline const std::vector<TransactionKernel>& GetKernels() const { return m_kernels; }
	inline const std::vector<ShortId>& GetShortIds() const { return m_shortIds; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static CompactBlock Deserialize(ByteBuffer& byteBuffer);

	//
	// Hashing
	//
	inline const Hash& Hash() const { return m_blockHeader.Hash(); }

private:
	BlockHeader m_blockHeader;
	uint64_t m_nonce;
	std::vector<TransactionOutput> m_outputs;
	std::vector<TransactionKernel> m_kernels;
	std::vector<ShortId> m_shortIds;
};