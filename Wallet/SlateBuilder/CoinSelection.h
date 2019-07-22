#pragma once

#include <Wallet/OutputData.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <set>

class CoinSelection
{
public:
	std::vector<OutputData> SelectCoinsToSpend(
		const std::vector<OutputData>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const ESelectionStrategy& strategy,
		const std::set<Commitment>& inputs,
		const int64_t numOutputs,
		const int64_t numKernels
	) const;

private:
	std::vector<OutputData> SelectUsingSmallestInputs(
		const std::vector<OutputData>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const int64_t numOutputs,
		const int64_t numKernels
	) const;

	std::vector<OutputData> SelectUsingCustomInputs(
		const std::vector<OutputData>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const std::set<Commitment>& inputs,
		const int64_t numOutputs,
		const int64_t numKernels
	) const;
};