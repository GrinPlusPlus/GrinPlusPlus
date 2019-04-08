#include "CoinSelection.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>
#include <Infrastructure/Logger.h>

// TODO: Apply Strategy instead of just selecting greatest number of outputs.
// If strategy is "ALL", spend all available coins to reduce the fee.
std::vector<OutputData> CoinSelection::SelectCoinsToSpend(const std::vector<OutputData>& availableCoins, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const
{
	std::vector<OutputData> sortedCoins = availableCoins;
	std::sort(sortedCoins.begin(), sortedCoins.end());

	uint64_t amountFound = 0;
	std::vector<OutputData> selectedCoins;
	for (const OutputData& coin : sortedCoins)
	{
		amountFound += coin.GetAmount();
		selectedCoins.push_back(coin);

		const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)selectedCoins.size(), numOutputs, numKernels);
		if (amountFound >= (amount + fee))
		{
			return selectedCoins;
		}
	}

	// Not enough coins found.
	LoggerAPI::LogError("SlateBuilder::SelectCoinsToSpend - Not enough funds.");
	throw InsufficientFundsException();
}