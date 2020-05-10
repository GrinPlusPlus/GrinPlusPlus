#pragma once

#include <Core/Exceptions/DeserializationException.h>
#include <string>

enum class EPostMethod
{
	FLUFF,
	STEM,
	JOIN
};

namespace PostMethod
{
	static EPostMethod FromString(const std::string& input)
	{
		if (input == "FLUFF")
		{
			return EPostMethod::FLUFF;
		}
		else if (input == "STEM")
		{
			return EPostMethod::STEM;
		}
		else if (input == "JOIN")
		{
			return EPostMethod::JOIN;
		}

		throw DESERIALIZATION_EXCEPTION_F("Invalid Post Method: {}", input);
	}
}