#pragma once

#include <stdint.h>

#include <Hash.h>
#include <Crypto.h>
#include <BigInteger.h>
#include <Core/ProofOfWork.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>
#include <HexUtil.h>

class BlockHeader
{
public:
	//
	// Constructors
	//
	BlockHeader(
		const uint16_t version,
		const uint64_t height,
		const int64_t timestamp,
		CBigInteger<32>&& previousBlockHash,
		CBigInteger<32>&& previousRoot,
		CBigInteger<32>&& outputRoot,
		CBigInteger<32>&& rangeProofRoot,
		CBigInteger<32>&& kernelRoot,
		BlindingFactor&& totalKernelOffset,
		const uint64_t outputMMRSize,
		const uint64_t kernelMMRSize,
		ProofOfWork&& proofOfWork
	);
	BlockHeader(const BlockHeader& other) = default;
	BlockHeader(BlockHeader&& other) noexcept = default;

	//
	// Destructor
	//
	~BlockHeader() = default;

	//
	// Operators
	//
	BlockHeader& operator=(const BlockHeader& other) = default;
	BlockHeader& operator=(BlockHeader&& other) noexcept = default;

	//
	// Getters
	//
	inline uint16_t GetVersion() const { return m_version; }
	inline uint64_t GetHeight() const { return m_height; }
	inline const CBigInteger<32>& GetPreviousBlockHash() const { return m_previousBlockHash; }
	inline const CBigInteger<32>& GetPreviousRoot() const { return m_previousRoot; }
	inline int64_t GetTimestamp() const { return m_timestamp; }
	inline const ProofOfWork& GetProofOfWork() const { return m_proofOfWork; }

	// Merklish roots
	inline const CBigInteger<32>& GetOutputRoot() const { return m_outputRoot; }
	inline const CBigInteger<32>& GetRangeProofRoot() const { return m_rangeProofRoot; }
	inline const CBigInteger<32>& GetKernelRoot() const { return m_kernelRoot; }

	inline const BlindingFactor& GetTotalKernelOffset() const { return m_totalKernelOffset; }

	// Merkle Mountain Range Sizes
	inline uint64_t GetOutputMMRSize() const { return m_outputMMRSize; }
	inline uint64_t GetKernelMMRSize() const { return m_kernelMMRSize; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static BlockHeader Deserialize(ByteBuffer& byteBuffer);

	//
	// Hashing
	//
	inline const Hash& Hash() const { return m_proofOfWork.Hash(); }
	inline const std::string FormatHash() const { return HexUtil::ConvertHash(Hash()); }

private:
	uint16_t m_version;
	uint64_t m_height;
	int64_t m_timestamp;
	CBigInteger<32> m_previousBlockHash;
	CBigInteger<32> m_previousRoot;
	CBigInteger<32> m_outputRoot;
	CBigInteger<32> m_rangeProofRoot;
	CBigInteger<32> m_kernelRoot;
	BlindingFactor m_totalKernelOffset;
	uint64_t m_outputMMRSize;
	uint64_t m_kernelMMRSize;
	ProofOfWork m_proofOfWork;
};