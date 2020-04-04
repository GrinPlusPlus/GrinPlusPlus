#pragma once

#include <string>

enum class EEnvironmentType
{
	MAINNET,
	FLOONET,
	AUTOMATED_TESTING
};

class Env
{
public:
	static std::string ToString(const EEnvironmentType env)
	{
		switch (env)
		{
			case EEnvironmentType::MAINNET: return "MAINNET";
			case EEnvironmentType::FLOONET: return "FLOONET";
			case EEnvironmentType::AUTOMATED_TESTING: return "AUTOMATED_TESTING";
		}

		throw std::exception();
	}
};