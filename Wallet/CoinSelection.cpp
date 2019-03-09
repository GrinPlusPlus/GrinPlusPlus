#include "CoinSelection.h"
#include "WalletUtil.h"

#include <Wallet/InsufficientFundsException.h>
#include <Infrastructure/Logger.h>

// TODO: Apply Strategy instead of just selecting greatest number of outputs.
// If strategy is "ALL", spend all available coins to reduce the fee.
std::vector<WalletCoin> CoinSelection::SelectCoinsToSpend(const std::vector<WalletCoin>& availableCoins, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const
{
	std::vector<WalletCoin> sortedCoins = availableCoins;
	std::sort(sortedCoins.begin(), sortedCoins.end());

	uint64_t amountFound = 0;
	std::vector<WalletCoin> selectedCoins;
	for (const WalletCoin& coin : sortedCoins)
	{
		amountFound += coin.GetOutputData().GetAmount();
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