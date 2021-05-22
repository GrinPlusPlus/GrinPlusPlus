#pragma once

#include <Core/Models/Features.h>
#include <Core/Config.h>
#include <Core/Global.h>
#include <Consensus.h>
#include <cstdint>

class WalletUtil
{
public:
	static bool IsOutputImmature(
		const EOutputFeatures features,
		const uint64_t output_height,
		const uint64_t current_height)
	{
		uint64_t min_confs = Global::GetConfig().GetMinimumConfirmations();
		if (features == EOutputFeatures::COINBASE_OUTPUT) {
			return Consensus::GetMaxCoinbaseHeight(current_height) <= output_height;
		} else {
			return (output_height + min_confs) > current_height;
		}
	}
};