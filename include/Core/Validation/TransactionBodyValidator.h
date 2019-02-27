#pragma once

#include <Core/Models/TransactionBody.h>

class TransactionBodyValidator
{
public:
	bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward);

private:
	bool ValidateWeight(const TransactionBody& transactionBody, const bool withReward);
	bool VerifySorted(const TransactionBody& transactionBody);
	bool VerifyCutThrough(const TransactionBody& transactionBody);
	bool VerifyRangeProofs(const std::vector<TransactionOutput>& outputs);
};