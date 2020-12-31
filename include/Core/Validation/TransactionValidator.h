#pragma once

#include <Core/Models/Transaction.h>

class TransactionValidator
{
public:
	void Validate(const Transaction& transaction, const uint64_t block_height) const;

private:
	void ValidateWeight(const TransactionBody& body, const uint64_t block_height) const;
	void ValidateFeatures(const TransactionBody& transactionBody) const;
	void ValidateKernelSums(const Transaction& transaction) const;
};