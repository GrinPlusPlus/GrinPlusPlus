#pragma once

#include <Core/Models/TransactionBody.h>
#include <memory>

class ICoinView
{
public:
    using CPtr = std::shared_ptr<const ICoinView>;

    ICoinView() = default;
    virtual ~ICoinView() = default;

    /// <summary>
    /// Looks up the type of the UTXO, given the output commitment.
    /// </summary>
    /// <param name="commitment">The commitment of the output to lookup.</param>
    /// <returns>The EOutputFeatures of the output.</returns>
    /// <throws>BlockChainException if not found</throws>
    virtual EOutputFeatures GetOutputType(const Commitment& commitment) const = 0;
};