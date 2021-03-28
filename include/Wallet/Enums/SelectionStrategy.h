#pragma once

#include <Core/Exceptions/DeserializationException.h>
#include <string>

enum class ESelectionStrategy
{
	SMALLEST,
	FEWEST_INPUTS,
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
		else if (input == "FEWEST_INPUTS")
		{
			return ESelectionStrategy::FEWEST_INPUTS;
		}
		else if (input == "CUSTOM")
		{
			return ESelectionStrategy::CUSTOM;
		}
		else if (input == "ALL")
		{
			return ESelectionStrategy::ALL;
		}

		throw DESERIALIZATION_EXCEPTION_F("Invalid selection strategy: {}", input);
	}

	static std::string ToString(const ESelectionStrategy& strategy)
	{
		if (strategy == ESelectionStrategy::SMALLEST)
		{
			return "SMALLEST";
		}
		else if (strategy == ESelectionStrategy::FEWEST_INPUTS)
		{
			return "FEWEST_INPUTS";
		}
		else if (strategy == ESelectionStrategy::CUSTOM)
		{
			return "CUSTOM";
		}
		else if (strategy == ESelectionStrategy::ALL)
		{
			return "ALL";
		}

		throw DESERIALIZATION_EXCEPTION("Invalid selection strategy");
	}
}