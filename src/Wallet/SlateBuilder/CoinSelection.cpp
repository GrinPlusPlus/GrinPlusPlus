#include "CoinSelection.h"

#include <Core/Util/FeeUtil.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>
#include <Common/Logger.h>

// If strategy is "ALL", spend all available coins to reduce the fee.
std::vector<OutputDataEntity> CoinSelection::SelectCoinsToSpend(
	const std::vector<OutputDataEntity>& availableCoins, 
	const uint64_t amount,
	const uint64_t feeBase,
	const ESelectionStrategy& strategy,
	const std::set<Commitment>& inputs,
	const int64_t numOutputs,
	const int64_t numKernels)
{
	if (strategy == ESelectionStrategy::CUSTOM)
	{
		return SelectUsingCustomInputs(availableCoins, amount, feeBase, inputs, numOutputs, numKernels);
	}
	else if (strategy == ESelectionStrategy::SMALLEST)
	{
		return SelectUsingSmallestInputs(availableCoins, amount, feeBase, numOutputs, numKernels);
	}
	else if (strategy == ESelectionStrategy::FEWEST_INPUTS)
	{
		return SelectUsingFewestInputs(availableCoins, amount, feeBase, numOutputs, numKernels);
	}
	else if (strategy == ESelectionStrategy::ALL)
	{
		return SelectUsingAllInputs(availableCoins, amount, feeBase, numOutputs, numKernels);
	}

	WALLET_ERROR("Unsupported selection strategy used.");
	throw InsufficientFundsException();
}

std::vector<OutputDataEntity> CoinSelection::SelectUsingSmallestInputs(
	const std::vector<OutputDataEntity>& availableCoins,
	const uint64_t amount,
	const uint64_t feeBase,
	const int64_t numOutputs,
	const int64_t numKernels)
{
	std::vector<OutputDataEntity> sortedCoins = availableCoins;
	std::sort(sortedCoins.begin(), sortedCoins.end());

	uint64_t amountFound = 0;
	std::vector<OutputDataEntity> selectedCoins;
	for (const OutputDataEntity& coin : sortedCoins)
	{
		amountFound += coin.GetAmount();
		selectedCoins.push_back(coin);

		const uint64_t fee = FeeUtil::CalculateFee(feeBase, (int64_t)selectedCoins.size(), numOutputs, numKernels);
		if (amountFound >= (amount + fee))
		{
			return selectedCoins;
		}
	}

	// Not enough coins found.
	WALLET_ERROR("Not enough funds.");
	throw InsufficientFundsException();
}

std::vector<OutputDataEntity> CoinSelection::SelectUsingFewestInputs(
	const std::vector<OutputDataEntity>& availableCoins,
	const uint64_t amount,
	const uint64_t feeBase,
	const int64_t numOutputs,
	const int64_t numKernels)
{
	std::vector<OutputDataEntity> sortedCoins = availableCoins;
	std::sort(sortedCoins.rbegin(), sortedCoins.rend());

	uint64_t amountFound = 0;
	std::vector<OutputDataEntity> selectedCoins;
	for (const OutputDataEntity& coin : sortedCoins) {
		amountFound += coin.GetAmount();
		selectedCoins.push_back(coin);

		const uint64_t fee = FeeUtil::CalculateFee(feeBase, (int64_t)selectedCoins.size(), numOutputs, numKernels);
		if (amountFound >= (amount + fee)) {
			return selectedCoins;
		}
	}

	// Not enough coins found.
	WALLET_ERROR("Not enough funds.");
	throw InsufficientFundsException();
}

std::vector<OutputDataEntity> CoinSelection::SelectUsingAllInputs(
	const std::vector<OutputDataEntity>& availableCoins,
	const uint64_t amount,
	const uint64_t feeBase,
	const int64_t numOutputs,
	const int64_t numKernels)
{
	uint64_t amountFound = 0;
	for (const OutputDataEntity& coin : availableCoins)
	{
		amountFound += coin.GetAmount();
	}

	const uint64_t fee = FeeUtil::CalculateFee(feeBase, (int64_t)availableCoins.size(), numOutputs, numKernels);
	if (amount < amount + fee) {
		return availableCoins;
	}

	// Not enough coins found.
	WALLET_ERROR("Not enough funds.");
	throw InsufficientFundsException();
}

std::vector<OutputDataEntity> CoinSelection::SelectUsingCustomInputs(
	const std::vector<OutputDataEntity>& availableCoins,
	const uint64_t amount,
	const uint64_t feeBase,
	const std::set<Commitment>& inputs,
	const int64_t numOutputs,
	const int64_t numKernels)
{
	uint64_t amountFound = 0;
	std::vector<OutputDataEntity> selectedCoins;
	for (const OutputDataEntity& coin : availableCoins)
	{
		if (inputs.find(coin.GetOutput().GetCommitment()) != inputs.end())
		{
			amountFound += coin.GetAmount();
			selectedCoins.push_back(coin);
		}
	}

	const uint64_t fee = FeeUtil::CalculateFee(feeBase, (int64_t)selectedCoins.size(), numOutputs, numKernels);
	if (amountFound >= (amount + fee))
	{
		return selectedCoins;
	}

	// Not enough coins selected.
	WALLET_ERROR("Not enough funds.");
	throw InsufficientFundsException();
}