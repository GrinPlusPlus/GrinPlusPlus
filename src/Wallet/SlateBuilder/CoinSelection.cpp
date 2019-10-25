#include "CoinSelection.h"

#include <Infrastructure/Logger.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>
#include <Wallet/WalletUtil.h>

// TODO: Apply Strategy instead of just selecting greatest number of outputs.
// If strategy is "ALL", spend all available coins to reduce the fee.
std::vector<OutputData> CoinSelection::SelectCoinsToSpend(const std::vector<OutputData> &availableCoins,
                                                          const uint64_t amount, const uint64_t feeBase,
                                                          const ESelectionStrategy &strategy,
                                                          const std::set<Commitment> &inputs, const int64_t numOutputs,
                                                          const int64_t numKernels) const
{
    if (strategy == ESelectionStrategy::CUSTOM)
    {
        return SelectUsingCustomInputs(availableCoins, amount, feeBase, inputs, numOutputs, numKernels);
    }
    else if (strategy == ESelectionStrategy::ALL)
    {
        return SelectUsingSmallestInputs(availableCoins, amount, feeBase, numOutputs, numKernels);
    }

    LoggerAPI::LogError("SlateBuilder::SelectCoinsToSpend - Unsupported selection strategy used.");
    throw InsufficientFundsException();
}

std::vector<OutputData> CoinSelection::SelectUsingSmallestInputs(const std::vector<OutputData> &availableCoins,
                                                                 const uint64_t amount, const uint64_t feeBase,
                                                                 const int64_t numOutputs,
                                                                 const int64_t numKernels) const
{
    std::vector<OutputData> sortedCoins = availableCoins;
    std::sort(sortedCoins.begin(), sortedCoins.end());

    uint64_t amountFound = 0;
    std::vector<OutputData> selectedCoins;
    for (const OutputData &coin : sortedCoins)
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
    LoggerAPI::LogError("SlateBuilder::SelectUsingSmallestInputs - Not enough funds.");
    throw InsufficientFundsException();
}

std::vector<OutputData> CoinSelection::SelectUsingCustomInputs(const std::vector<OutputData> &availableCoins,
                                                               const uint64_t amount, const uint64_t feeBase,
                                                               const std::set<Commitment> &inputs,
                                                               const int64_t numOutputs, const int64_t numKernels) const
{
    uint64_t amountFound = 0;
    std::vector<OutputData> selectedCoins;
    for (const OutputData &coin : availableCoins)
    {
        if (inputs.find(coin.GetOutput().GetCommitment()) != inputs.end())
        {
            amountFound += coin.GetAmount();
            selectedCoins.push_back(coin);
        }
    }

    const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)selectedCoins.size(), numOutputs, numKernels);
    if (amountFound >= (amount + fee))
    {
        return selectedCoins;
    }

    // Not enough coins selected.
    LoggerAPI::LogError("SlateBuilder::SelectUsingCustomInputs - Not enough funds.");
    throw InsufficientFundsException();
}