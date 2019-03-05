#pragma once

#include <Core/Serialization/DeserializationException.h>
#include <string>

enum class ESelectionStrategy
{
	ALL
};

namespace SelectionStrategy
{
	static ESelectionStrategy FromString(const std::string& input)
	{
		if (input == "ALL")
		{
			return ESelectionStrategy::ALL;
		}

		throw DeserializationException();
	}
}