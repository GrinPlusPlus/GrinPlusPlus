#pragma once

#include <Core/Models/Transaction.h>

class TransactionValidator
{
public:
	void Validate(const Transaction& transaction) const;

private:
	void ValidateFeatures(const TransactionBody& transactionBody) const;
	void ValidateKernelSums(const Transaction& transaction) const;
};