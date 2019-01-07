#pragma once

#include <Core/Transaction.h>
#include <vector>

class TransactionAggregator
{
public:
	static std::unique_ptr<Transaction> Aggregate(const std::vector<Transaction>& transactions);
	static std::unique_ptr<Transaction> Deaggregate(const Transaction& multiKernelTx, const std::vector<Transaction>& transactions);

private:
	static void PerformCutThrough(std::vector<TransactionInput>& inputs, std::vector<TransactionOutput>& outputs);
};