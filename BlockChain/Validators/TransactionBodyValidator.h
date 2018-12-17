#pragma once

#include <Core/TransactionBody.h>

class TransactionBodyValidator
{
public:
	bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const;

private:
	bool ValidateWeight(const TransactionBody& transactionBody, const bool withReward) const;
	bool VerifySorted(const TransactionBody& transactionBody) const;
	bool VerifyCutThrough(const TransactionBody& transactionBody) const;
	bool VerifyOutputs(const std::vector<TransactionOutput>& outputs) const;
	bool VerifyKernels(const std::vector<TransactionKernel>& kernels) const;
};