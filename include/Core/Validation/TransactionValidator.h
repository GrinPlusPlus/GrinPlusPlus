#pragma once

#include <Core/Models/Transaction.h>

class TransactionValidator
{
public:
	bool ValidateTransaction(const Transaction& transaction);

private:
	bool ValidateFeatures(const TransactionBody& transactionBody);
	bool ValidateKernelSums(const Transaction& transaction);
};