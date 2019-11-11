#pragma once

#include <Wallet/OutputData.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <set>

class CoinSelection
{
public:
	static std::vector<OutputData> SelectCoinsToSpend(
		const std::vector<OutputData>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const ESelectionStrategy& strategy,
		const std::set<Commitment>& inputs,
		const int64_t numOutputs,
		const int64_t numKernels
	);

private:
	static std::vector<OutputData> SelectUsingSmallestInputs(
		const std::vector<OutputData>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const int64_t numOutputs,
		const int64_t numKernels
	);

	static std::vector<OutputData> SelectUsingCustomInputs(
		const std::vector<OutputData>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const std::set<Commitment>& inputs,
		const int64_t numOutputs,
		const int64_t numKernels
	);
};