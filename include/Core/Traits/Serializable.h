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
	};
}