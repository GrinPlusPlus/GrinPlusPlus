#pragma once

#include <Wallet/OutputData.h>
#include <Wallet/SelectionStrategy.h>

class CoinSelection
{
public:
	std::vector<OutputData> SelectCoinsToSpend(const std::vector<OutputData>& availableCoins, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const;
};