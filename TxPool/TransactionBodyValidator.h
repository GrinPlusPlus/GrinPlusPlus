#pragma once

#include <Core/Models/TransactionBody.h>
#include <lru/cache.hpp>

class TransactionBodyValidator
{
public:
	TransactionBodyValidator(LRU::Cache<Commitment, Commitment>& bulletproofsCache);

	bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward);

private:
	bool ValidateWeight(const TransactionBody& transactionBody, const bool withReward);
	bool VerifySorted(const TransactionBody& transactionBody);
	bool VerifyCutThrough(const TransactionBody& transactionBody);
	bool VerifyOutputs(const std::vector<TransactionOutput>& outputs);
	bool VerifyKernels(const std::vector<TransactionKernel>& kernels);

	LRU::Cache<Commitment, Commitment>& m_bulletproofsCache;
};