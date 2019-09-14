#include "PoWUtil.h"

PoWUtil::PoWUtil(const Config& config)
	: m_config(config)
{

}

EPoWType PoWUtil::DeterminePoWType(const uint64_t headerVersion, const uint8_t edgeBits) const
{
	// MAINNET: Check environment
	if (edgeBits == 29)
	{
		if (headerVersion == 1)
		{
			return EPoWType::CUCKAROO;
		}
		else
		{
			return EPoWType::CUCKAROOD;
		}
	}
	else
	{
		return EPoWType::CUCKATOO;
	}
}