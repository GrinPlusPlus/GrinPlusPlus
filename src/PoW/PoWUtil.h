#pragma once

#include "PoWType.h"

#include <Config/Config.h>
#include <stdint.h>

class PoWUtil
{
public:
	PoWUtil(const Config& config);

	EPoWType DeterminePoWType(const uint64_t headerVersion, const uint8_t edgeBits) const;

private:
	const Config& m_config;
};