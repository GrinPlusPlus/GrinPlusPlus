#pragma once

#include <Crypto/Models/Hash.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>
#include <PMMR/Common/SegmentId.h>
#include <PMMR/Common/SegmentProof.h>
#include <cstdint>

template<size_t DATA_SIZE, class DATA_TYPE>
class Segment : public Traits::ISerializable
{
	SegmentIdentifier m_id;
	std::vector<uint64_t> m_hash_pos;
	std::vector<Hash> m_hashes;
	std::vector<uint64_t> m_leaf_pos;
	std::vector<DATA_TYPE> m_leaves;
	SegmentProof m_proof;

public:
	//
	// Constructors
	//
	Segment(SegmentIdentifier id) : m_id(std::move(id)) {}
	Segment(const Segment& other) = default;
	Segment(Segment&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~Segment() = default;

	//
	// Operators
	//
	Segment& operator=(const Segment& other) = default;
	Segment& operator=(Segment&& other) noexcept = default;

	//
	// Getters
	//
	uint8_t GetHeight() const noexcept { return m_height; }
	uint64_t GetIndex() const noexcept { return m_index; }


	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final
	{
		serializer.Append(m_id);
		serializer.Append<uint64_t>(m_hash_pos.size());
		for (const uint64_t pos : m_hash_pos) {
			serializer.Append<uint64_t>(pos);
		}
		for (const Hash& hash : m_hashes) {
			serializer.AppendBigInteger<32>(hash);
		}
		serializer.Append<uint64_t>(m_leaf_pos.size());
		for (const uint64_t pos : m_leaf_pos) {
			serializer.Append<uint64_t>(pos);
		}
		for (const DATA_TYPE& leaf : m_leaves) {
			leaf.Serialize(serializer);
		}
		m_proof.Serialize(serializer);
	}

	static Segment Deserialize(ByteBuffer& byteBuffer)
	{
		SegmentIdentifier segmentId = SegmentIdentifier::Deserialize(byteBuffer);
		const uint64_t numHashes = byteBuffer.Read<uint64_t>();
		std::vector<uint64_t> hashPos(numHashes);
		for (uint64_t i = 0; i < numHashes; ++i) {
			hashPos[i] = byteBuffer.Read<uint64_t>();
		}
		std::vector<Hash> hashes;
		hashes.reserve(numHashes);
		for (uint64_t i = 0; i < numHashes; ++i) {
			hashes.push_back(byteBuffer.ReadBigInteger<32>());
		}
		const uint64_t numLeaves = byteBuffer.Read<uint64_t>();
		std::vector<uint64_t> leafPos(numLeaves);
		for (uint64_t i = 0; i < numLeaves; ++i) {
			leafPos[i] = byteBuffer.Read<uint64_t>();
		}
		std::vector<DATA_TYPE> leaves;
		leaves.reserve(numLeaves);
		for (uint64_t i = 0; i < numLeaves; ++i) {
			leaves.push_back(DATA_TYPE::Deserialize(byteBuffer));
		}
		SegmentProof segmentProof = SegmentProof::Deserialize(byteBuffer);
		return Segment(std::move(segmentId), std::move(hashPos), std::move(hashes), std::move(leafPos), std::move(leaves), std::move(segmentProof));
	}
};