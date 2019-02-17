#pragma once

#include <Core/Models/Transaction.h>
#include <vector>

class TransactionAggregator
{
public:
	static std::unique_ptr<Transaction> Aggregate(const std::vector<Transaction>& transactions);
	static std::unique_ptr<Transaction> Deaggregate(const Transaction& multiKernelTx, const std::vector<Transaction>& transactions);
};