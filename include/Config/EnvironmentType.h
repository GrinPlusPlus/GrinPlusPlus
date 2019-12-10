#pragma once

#include <string>

enum class EEnvironmentType
{
	MAINNET,
	FLOONET
};

namespace Env
{
	static std::string ToString(const EEnvironmentType env)
	{
		return env == EEnvironmentType::MAINNET ? "MAINNET" : "FLOONET";
	}
}