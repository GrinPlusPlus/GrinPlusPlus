#pragma once

#include "WalletCoin.h"

#include <Wallet/SelectionStrategy.h>

class CoinSelection
{
public:
	std::vector<WalletCoin> SelectCoinsToSpend(const std::vector<WalletCoin>& availableCoins, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const;
};