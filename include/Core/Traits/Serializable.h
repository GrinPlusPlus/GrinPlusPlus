#pragma once

#include <Core/Serialization/Serializer.h>

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

		std::vector<unsigned char> Serialized(const EProtocolVersion version = EProtocolVersion::V1) const
		{
			Serializer serializer(version);
			Serialize(serializer);
			return serializer.GetBytes();
		}

		virtual std::vector<unsigned char> SerializeWithIndex(const uint64_t index) const
		{
			Serializer serializer;
			serializer.Append<uint64_t>(index);
			Serialize(serializer);
			return serializer.GetBytes();
		}
	};
}