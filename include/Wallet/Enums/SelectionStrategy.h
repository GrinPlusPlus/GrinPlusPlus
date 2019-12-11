#pragma once

#include <Core/Exceptions/DeserializationException.h>
#include <string>

enum class ESelectionStrategy
{
	SMALLEST,
	CUSTOM,
	ALL
};

namespace SelectionStrategy
{
	static ESelectionStrategy FromString(const std::string& input)
	{
		if (input == "SMALLEST")
		{
			return ESelectionStrategy::SMALLEST;
		}
		else if (input == "CUSTOM")
		{
			return ESelectionStrategy::CUSTOM;
		}
		else if (input == "ALL")
		{
			return ESelectionStrategy::ALL;
		}

		throw DESERIALIZATION_EXCEPTION();
	}
}