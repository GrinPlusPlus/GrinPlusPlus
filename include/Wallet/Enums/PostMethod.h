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
	static std::string ToString(const EPostMethod method)
	{
		switch (method)
		{
		case EPostMethod::FLUFF:
			return "FLUFF";
		case EPostMethod::STEM:
			return "STEM";
		case EPostMethod::JOIN:
			return "JOIN";
		}

		throw std::exception();
	}

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