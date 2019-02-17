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

	ProofOfWork(const uint8_t edgeBits, std::vector<uint64_t>&& proofNonces);
	ProofOfWork(const uint8_t edgeBits, std::vector<uint64_t>&& proofNonces, Hash&& hash);
	ProofOfWork(const ProofOfWork& other) = default;
	ProofOfWork(ProofOfWork&& other) noexcept = default;

	ProofOfWork& operator=(const ProofOfWork& other) = default;
	ProofOfWork& operator=(ProofOfWork&& other) noexcept = default;

	////////////////////////////////////////
	// STANDARD POW ATTRIBUTES
	////////////////////////////////////////

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
	const Hash& GetHash() const; // TODO: Rename "GetCycleHash"
	void SerializeProofNonces(Serializer& serializer) const; // TODO: Should be private

private:
	static std::vector<uint64_t> DeserializeProofNonces(const std::vector<unsigned char>& bits, const uint8_t edgeBits);

	uint8_t m_edgeBits;
	std::vector<uint64_t> m_proofNonces;
	mutable Hash m_hash;
};