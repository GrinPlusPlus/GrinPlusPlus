#pragma once

#include <Core/Transaction.h>

class TransactionValidator
{
public:
	bool ValidateTransaction(const Transaction& transaction) const;

private:
	bool ValidateFeatures(const TransactionBody& transactionBody) const;
	bool ValidateKernelSums(const Transaction& transaction) const;
};