#pragma once

#include <Crypto/Models/BlindingFactor.h>
#include <Crypto/Models/Hash.h>
#include <Crypto/Models/BigInteger.h>
#include <Core/Models/ProofOfWork.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Printable.h>
#include <Core/Traits/Hashable.h>
#include <Core/Traits/Serializable.h>
#include <Common/Util/HexUtil.h>
#include <PMMR/Common/Index.h>
#include <json/json.h>
#include <cstdint>
#include <memory>

class BlockHeader : public Traits::IPrintable, public Traits::ISerializable, public Traits::IHashable
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
	const Hash& GetPreviousHash() const { return m_previousBlockHash; }
	const Hash& GetPreviousRoot() const { return m_previousRoot; }
	int64_t GetTimestamp() const { return m_timestamp; }
	uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	uint32_t GetScalingDifficulty() const { return m_scalingDifficulty; }
	uint64_t GetNonce() const { return m_nonce; }

	// PoW
	const ProofOfWork& GetProofOfWork() const { return m_proofOfWork; }
	uint8_t GetEdgeBits() const { return m_proofOfWork.GetEdgeBits(); }
	const std::vector<uint64_t>& GetProofNonces() const { return m_proofOfWork.GetProofNonces(); }
	bool IsPrimaryPoW() const { return m_proofOfWork.IsPrimary(); }
	bool IsSecondaryPoW() const { return m_proofOfWork.IsSecondary(); }

	// Merklish roots
	const Hash& GetOutputRoot() const { return m_outputRoot; }
	const Hash& GetRangeProofRoot() const { return m_rangeProofRoot; }
	const Hash& GetKernelRoot() const { return m_kernelRoot; }

	const BlindingFactor& GetTotalKernelOffset() const { return m_totalKernelOffset; }
	const BlindingFactor& GetOffset() const { return m_totalKernelOffset; }

	// Merkle Mountain Range Sizes
	uint64_t GetOutputMMRSize() const { return m_outputMMRSize; }
	uint64_t GetKernelMMRSize() const { return m_kernelMMRSize; }
	uint64_t GetNumOutputs() const { return Index::At(m_outputMMRSize).GetLeafIndex(); }
	uint64_t GetNumKernels() const { return Index::At(m_kernelMMRSize).GetLeafIndex(); }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final;
	static BlockHeader Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON() const;
	static std::shared_ptr<BlockHeader> FromJSON(const Json::Value& json);
	std::vector<unsigned char> GetPreProofOfWork() const;

	//
	// Hashing
	//
	const Hash& GetHash() const { return m_proofOfWork.GetHash(); }
	std::string ShortHash() const { return HASH::ShortHash(GetHash()); }

	//
	// Traits
	//
 	std::string Format() const final { return GetHash().ToHex(); }
	std::vector<unsigned char> SerializeWithIndex(const uint64_t index) const final
	{
		Serializer serializer;
		serializer.Append<uint64_t>(index);
		m_proofOfWork.SerializeCycle(serializer);
		return serializer.GetBytes();
	}

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