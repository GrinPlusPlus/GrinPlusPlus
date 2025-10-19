#pragma once

#include <Crypto/Models/Hash.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>
#include <cstdint>

class SegmentProof : public Traits::ISerializable
{
	std::vector<Hash> m_hashes;

public:
	//
	// Constructors
	//
	SegmentProof(std::vector<Hash> hashes) : m_hashes(std::move(hashes)) {}
	SegmentProof(const SegmentProof& other) = default;
	SegmentProof(SegmentProof&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~SegmentProof() = default;

	//
	// Operators
	//
	SegmentProof& operator=(const SegmentProof& other) = default;
	SegmentProof& operator=(SegmentProof&& other) noexcept = default;

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
		serializer.Append<uint64_t>(m_hashes.size());
		for (const auto& hash : m_hashes) {
			serializer.AppendHash(hash);
		}
	}

	static SegmentProof Deserialize(ByteBuffer& byteBuffer)
	{
		uint64_t size = byteBuffer.ReadUInt64();
		std::vector<Hash> hashes;
		hashes.reserve(size);
		for (uint64_t i = 0; i < size; ++i) {
			hashes.push_back(byteBuffer.ReadHash());
		}
		return SegmentProof(std::move(hashes));
	}
};