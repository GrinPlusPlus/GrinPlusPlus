#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/ShortId.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <json/json.h>

class CompactBlock
{
public:
	//
	// Constructors
	//
	CompactBlock(
		BlockHeaderPtr pBlockHeader,
		const uint64_t nonce,
		std::vector<TransactionOutput>&& fullOutputs,
		std::vector<TransactionKernel>&& fullKernels,
		std::vector<ShortId>&& shortIds
	);
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
	const BlockHeaderPtr& GetHeader() const { return m_pBlockHeader; }
	uint64_t GetNonce() const { return m_nonce; }

	const std::vector<TransactionOutput>& GetOutputs() const { return m_outputs; }
	const std::vector<TransactionKernel>& GetKernels() const { return m_kernels; }
	const std::vector<ShortId>& GetShortIds() const { return m_shortIds; }

	const Hash& GetPreviousHash() const { return m_pBlockHeader->GetPreviousHash(); }
	uint64_t GetHeight() const { return m_pBlockHeader->GetHeight(); }
	uint64_t GetTotalDifficulty() const { return m_pBlockHeader->GetTotalDifficulty(); }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static CompactBlock Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON() const;

	//
	// Hashing
	//
	const Hash& GetHash() const { return m_pBlockHeader->GetHash(); }

private:
	BlockHeaderPtr m_pBlockHeader;
	uint64_t m_nonce;
	std::vector<TransactionOutput> m_outputs;
	std::vector<TransactionKernel> m_kernels;
	std::vector<ShortId> m_shortIds;
};