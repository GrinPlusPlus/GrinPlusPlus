#pragma once

#include <Core/Enums/ProtocolVersion.h>

#include <cstdint>
#include <vector>

// Forward Declarations
class Serializer;

namespace Traits
{
	class ISerializable
	{
	public:
		virtual ~ISerializable() = default;

		//
		// Appends serialized bytes to Serializer
		//
		virtual void Serialize(Serializer& serializer) const = 0;

		std::vector<uint8_t> Serialized(const EProtocolVersion version = EProtocolVersion::V1) const;

		virtual std::vector<uint8_t> SerializeWithIndex(const uint64_t index) const;
	};
}