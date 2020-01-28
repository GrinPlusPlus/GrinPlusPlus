#pragma once

#include <stdint.h>

#include <Crypto/BlindingFactor.h>
#include <Crypto/Hash.h>
#include <Crypto/BigInteger.h>
#include <Core/Models/ProofOfWork.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Printable.h>
#include <Core/Traits/Serializable.h>
#include <Common/Util/HexUtil.h>
#include <memory>

class BlockHeader : public Traits::IPrintable, public Traits::ISerializable
{
public:
	//
	// Constructors
	//
	BlockHeader(
		const uint16_t version,
		const uint64_t height,
		const int64_t timestamp,
		Hash&& previousBlockHash,
		Hash&& previousRoot,
		Hash&& outputRoot,
		Hash&& rangeProofRoot,
		Hash&& kernelRoot,
		BlindingFactor&& totalKernelOffset,
		const uint64_t outputMMRSize,
		const uint64_t kernelMMRSize,
		const uint64_t totalDifficulty,
		const uint32_t scalingDifficulty,
		const uint64_t nonce,
		ProofOfWork&& proofOfWork
	);
	BlockHeader(const BlockHeader& other) = default;
	BlockHeader(BlockHeader&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~BlockHeader() = default;

	//
	// Operators
	//
	BlockHeader& operator=(const BlockHeader& other) = default;
	BlockHeader& operator=(BlockHeader&& other) noexcept = default;
	bool operator!=(const BlockHeader& rhs) const { return this->GetHash() != rhs.GetHash(); }

	//
	// Getters
	//
	uint16_t GetVersion() const { return m_version; }
	uint64_t GetHeight() const { return m_height; }
	const Hash& GetPreviousBlockHash() const { return m_previousBlockHash; }
	const Hash& GetPreviousRoot() const { return m_previousRoot; }
	int64_t GetTimestamp() const { return m_timestamp; }
	uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	uint32_t GetScalingDifficulty() const { return m_scalingDifficulty; }
	uint64_t GetNonce() const { return m_nonce; }
	const ProofOfWork& GetProofOfWork() const { return m_proofOfWork; }

	// Merklish roots
	const Hash& GetOutputRoot() const { return m_outputRoot; }
	const Hash& GetRangeProofRoot() const { return m_rangeProofRoot; }
	const Hash& GetKernelRoot() const { return m_kernelRoot; }

	const BlindingFactor& GetTotalKernelOffset() const { return m_totalKernelOffset; }

	// Merkle Mountain Range Sizes
	uint64_t GetOutputMMRSize() const { return m_outputMMRSize; }
	uint64_t GetKernelMMRSize() const { return m_kernelMMRSize; }

	//
	// Serialization/Deserialization
	//
	virtual void Serialize(Serializer& serializer) const override final;
	static BlockHeader Deserialize(ByteBuffer& byteBuffer);
	std::vector<unsigned char> GetPreProofOfWork() const;

	//
	// Hashing
	//
	const Hash& GetHash() const { return m_proofOfWork.GetHash(); }
	std::string ShortHash() const { return HASH::ShortHash(GetHash()); }

	//
	// Traits
	//
 	virtual std::string Format() const override final { return GetHash().ToHex(); }

private:
	uint16_t m_version;
	uint64_t m_height;
	int64_t m_timestamp;
	Hash m_previousBlockHash;
	Hash m_previousRoot;
	Hash m_outputRoot;
	Hash m_rangeProofRoot;
	Hash m_kernelRoot;
	BlindingFactor m_totalKernelOffset;
	uint64_t m_outputMMRSize;
	uint64_t m_kernelMMRSize;
	uint64_t m_totalDifficulty;
	uint32_t m_scalingDifficulty;
	uint64_t m_nonce;
	ProofOfWork m_proofOfWork;
};

typedef std::shared_ptr<const BlockHeader> BlockHeaderPtr;