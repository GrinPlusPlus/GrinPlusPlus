#pragma once

#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <set>

class CoinSelection
{
public:
	static std::vector<OutputDataEntity> SelectCoinsToSpend(
		const std::vector<OutputDataEntity>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const ESelectionStrategy& strategy,
		const std::set<Commitment>& inputs,
		const int64_t numOutputs,
		const int64_t numKernels
	);

private:
	static std::vector<OutputDataEntity> SelectUsingSmallestInputs(
		const std::vector<OutputDataEntity>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const int64_t numOutputs,
		const int64_t numKernels
	);

	static std::vector<OutputDataEntity> SelectUsingFewestInputs(
		const std::vector<OutputDataEntity>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const int64_t numOutputs,
		const int64_t numKernels
	);

	static std::vector<OutputDataEntity> SelectUsingAllInputs(
		const std::vector<OutputDataEntity>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const int64_t numOutputs,
		const int64_t numKernels
	);

	static std::vector<OutputDataEntity> SelectUsingCustomInputs(
		const std::vector<OutputDataEntity>& availableCoins,
		const uint64_t amount,
		const uint64_t feeBase,
		const std::set<Commitment>& inputs,
		const int64_t numOutputs,
		const int64_t numKernels
	);
};