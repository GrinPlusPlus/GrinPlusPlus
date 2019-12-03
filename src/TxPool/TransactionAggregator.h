#pragma once

#include <Core/Models/Transaction.h>
#include <vector>

class TransactionAggregator
{
public:
	//
	// Aggregates multiple transactions into 1.
	//
	// Preconditions: transactions must not be empty
	//
	static TransactionPtr Aggregate(const std::vector<TransactionPtr>& transactions);
};