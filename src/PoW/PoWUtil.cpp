#include "PoWUtil.h"

#include <cassert>

PoWUtil::PoWUtil(const Config& config)
	: m_config(config)
{

}

EPoWType PoWUtil::DeterminePoWType(const uint64_t headerVersion, const uint8_t edgeBits) const
{
	assert(headerVersion <= 3);

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
		else if (headerVersion == 3)
		{
			return EPoWType::CUCKAROOM;
		}
		else
		{
			return EPoWType::CUCKAROOZ;
		}
	}
	else
	{
		return EPoWType::CUCKATOO;
	}
}