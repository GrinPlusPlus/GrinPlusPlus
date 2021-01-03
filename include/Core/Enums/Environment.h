#pragma once

#include <string>

enum class Environment
{
	MAINNET,
	FLOONET,
	AUTOMATED_TESTING
};

class Env
{
public:
	static std::string ToString(const Environment env)
	{
		switch (env)
		{
			case Environment::MAINNET: return "MAINNET";
			case Environment::FLOONET: return "FLOONET";
			case Environment::AUTOMATED_TESTING: return "AUTOMATED_TESTING";
		}

		throw std::exception();
	}
};