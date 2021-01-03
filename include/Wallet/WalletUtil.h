#pragma once

#include <Core/Models/Features.h>
#include <Consensus.h>
#include <cstdint>

class WalletUtil
{
public:
	static bool IsOutputImmature(
		const EOutputFeatures features,
		const uint64_t outputBlockHeight,
		const uint64_t currentBlockHeight,
		const uint64_t minimumConfirmations)
	{
		if (features == EOutputFeatures::COINBASE_OUTPUT) {
			return Consensus::GetMaxCoinbaseHeight(currentBlockHeight) <= outputBlockHeight;
		} else {
			return (outputBlockHeight + minimumConfirmations) > currentBlockHeight;
		}
	}
};