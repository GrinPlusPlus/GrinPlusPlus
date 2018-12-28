#pragma once

#include <Core/Transaction.h>
#include <vector>

class TransactionAggregator
{
public:
	static Transaction Aggregate(const std::vector<Transaction>& transactions);
	static Transaction Deaggregate(const Transaction& multiKernelTx, const std::vector<Transaction>& transactions);
};