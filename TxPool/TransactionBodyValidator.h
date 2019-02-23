#pragma once

#include "BulletProofsCache.h"

#include <Core/Models/TransactionBody.h>

class TransactionBodyValidator
{
public:
	TransactionBodyValidator(BulletProofsCache& bulletproofsCache);

	bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward);

private:
	bool ValidateWeight(const TransactionBody& transactionBody, const bool withReward);
	bool VerifySorted(const TransactionBody& transactionBody);
	bool VerifyCutThrough(const TransactionBody& transactionBody);
	bool VerifyOutputs(const std::vector<TransactionOutput>& outputs);

	BulletProofsCache& m_bulletproofsCache;
};