#pragma once

#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>
#include <cstdint>

class SegmentIdentifier : public Traits::ISerializable
{
	uint8_t m_height;
	uint64_t m_index;

public:
	//
	// Constructors
	//
	SegmentIdentifier() : m_height(0), m_index(0) {}
	SegmentIdentifier(uint8_t height, uint64_t index) : m_height(height), m_index(index) {}
	SegmentIdentifier(const SegmentIdentifier& other) = default;
	SegmentIdentifier(SegmentIdentifier&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~SegmentIdentifier() = default;

	//
	// Operators
	//
	SegmentIdentifier& operator=(const SegmentIdentifier& other) = default;
	SegmentIdentifier& operator=(SegmentIdentifier&& other) noexcept = default;

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
		serializer.Append(m_height);
		serializer.Append(m_index);
	}

	static SegmentIdentifier Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t height = byteBuffer.Read<uint8_t>();
		const uint64_t index = byteBuffer.Read<uint64_t>();
		return SegmentIdentifier(height, index);
	}
};