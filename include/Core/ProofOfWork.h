#pragma once

#include <stdint.h>
#include <Hash.h>
#include <BigInteger.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

class ProofOfWork
{
public:
	////////////////////////////////////////
	// CONSTRUCTION/DESTRUCTION
	////////////////////////////////////////

	ProofOfWork(const uint64_t totalDifficulty, const uint32_t scalingDifficulty, const uint64_t nonce, const uint8_t edgeBits, std::vector<uint64_t>&& proofNonces);
	ProofOfWork(const ProofOfWork& other) = default;
	ProofOfWork(ProofOfWork&& other) noexcept = default;

	ProofOfWork& operator=(const ProofOfWork& other) = default;
	ProofOfWork& operator=(ProofOfWork&& other) noexcept = default;

	////////////////////////////////////////
	// STANDARD POW ATTRIBUTES
	////////////////////////////////////////

	inline uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	inline uint32_t GetScalingDifficulty() const { return m_scalingDifficulty; }
	inline uint64_t GetNonce() const { return m_nonce; }
	inline uint8_t GetEdgeBits() const { return m_edgeBits; }
	inline const std::vector<uint64_t>& GetProofNonces() const { return m_proofNonces; }

	////////////////////////////////////////
	// SERIALIZATION/DESERIALIZATION
	////////////////////////////////////////

	void Serialize(Serializer& serializer) const;
	static ProofOfWork Deserialize(ByteBuffer& byteBuffer);

	////////////////////////////////////////
	// HASHING
	////////////////////////////////////////
	const Hash& GetHash() const;
	void SerializeProofNonces(Serializer& serializer) const; // TODO: Should be private

private:
	static std::vector<uint64_t> DeserializeProofNonces(ByteBuffer& byteBuffer, const uint8_t edgeBits);

	uint64_t m_totalDifficulty;
	uint32_t m_scalingDifficulty;
	uint64_t m_nonce;
	uint8_t m_edgeBits;
	std::vector<uint64_t> m_proofNonces;
	mutable CBigInteger<32> m_hash;
};