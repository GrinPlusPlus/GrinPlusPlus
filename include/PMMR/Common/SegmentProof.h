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
	SegmentProof() = default;
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
	const std::vector<Hash>& GetHashes() const noexcept { return m_hashes; }
	bool Empty() const noexcept { return m_hashes.empty(); }


	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final
	{
		serializer.Append<uint64_t>(m_hashes.size());
		for (const auto& hash : m_hashes) {
			serializer.AppendBigInteger<32>(hash);
		}
	}

	static SegmentProof Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t size = byteBuffer.Read<uint64_t>();
		std::vector<Hash> hashes;
		hashes.reserve(size);
		for (uint64_t i = 0; i < size; ++i) {
			hashes.push_back(byteBuffer.ReadBigInteger<32>());
		}
		return SegmentProof(std::move(hashes));
	}
};