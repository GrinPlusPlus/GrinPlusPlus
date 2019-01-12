#include "PoWUtil.h"

PoWUtil::PoWUtil(const Config& config)
	: m_config(config)
{

}

EPoWType PoWUtil::DeterminePoWType(const uint8_t edgeBits) const
{
	// MAINNET: Check environment
	if (edgeBits == 29)
	{
		return EPoWType::CUCKAROO;
	}
	else
	{
		return EPoWType::CUCKATOO;
	}
}