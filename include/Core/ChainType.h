#pragma once

#include <stdint.h>

enum class EChainType
{
	// Chain of validated headers used during sync. May not have the most work.
	SYNC,

	// Chain of validated headers containing the most work.
	CANDIDATE,

	// Chain of headers with fully validated blocks. 
	// Shouldn't differ from candidate chain, aside from often being shorter.
	CONFIRMED
};

namespace ChainType
{
	static const uint8_t NONE_MASK = 0x00;
	static const uint8_t SYNC_MASK = 0x01;
	static const uint8_t CANDIDATE_MASK = 0x02;
	static const uint8_t CONFIRMED_MASK = 0x04;
	static const uint8_t ALL_MASK = 0x07;

	static uint8_t GetMask(const EChainType chainType)
	{
		if (chainType == EChainType::SYNC)
		{
			return SYNC_MASK;
		}
		else if (chainType == EChainType::CANDIDATE)
		{
			return CANDIDATE_MASK;
		}
		else if (chainType == EChainType::CONFIRMED)
		{
			return CONFIRMED_MASK;
		}

		return NONE_MASK;
	}

	static bool IsSync(const uint8_t chainTypeMask) 
	{ 
		return (chainTypeMask & SYNC_MASK) == SYNC_MASK;
	}

	static bool IsCandidate(const uint8_t chainTypeMask)
	{ 
		return (chainTypeMask & CANDIDATE_MASK) == CANDIDATE_MASK;
	}

	static bool IsConfirmed(const uint8_t chainTypeMask)
	{ 
		return (chainTypeMask & CONFIRMED_MASK) == CONFIRMED_MASK;
	}

	static bool IsNone(const uint8_t chainTypeMask)
	{
		return chainTypeMask == NONE_MASK;
	}

	static bool IsAll(const uint8_t chainTypeMask)
	{
		return chainTypeMask == ALL_MASK;
	}

	static uint8_t AddChainType(const uint8_t chainTypeMask, const EChainType chainType)
	{
		return chainTypeMask | GetMask(chainType);
	}

	static uint8_t RemoveChainType(const uint8_t chainTypeMask, const EChainType chainType)
	{
		return chainTypeMask & (~GetMask(chainType));
	}
}