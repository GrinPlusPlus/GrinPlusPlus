#include "PoWUtil.h"

#include <cassert>

PoWUtil::PoWUtil(const Config& config)
	: m_config(config)
{

}

EPoWType PoWUtil::DeterminePoWType(const uint64_t headerVersion, const uint8_t edgeBits) const
{
	assert(headerVersion <= 3);

	// MAINNET: Check environment
	if (edgeBits == 29)
	{
		if (headerVersion == 1)
		{
			return EPoWType::CUCKAROO;
		}
		else if (headerVersion == 2)
		{
			return EPoWType::CUCKAROOD;
		}
		else
		{
			return EPoWType::CUCKAROOM;
		}
	}
	else
	{
		return EPoWType::CUCKATOO;
	}
}