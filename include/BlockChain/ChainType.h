#pragma once

#include <cstdint>

enum class EChainType
{
	// Chain of validated headers containing the most work.
	CANDIDATE,

	// Chain of headers with fully validated blocks. 
	// Shouldn't differ from candidate chain, aside from often being shorter.
	CONFIRMED
};