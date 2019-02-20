#pragma once

#include "TransactionBodyValidator.h"

#include <Core/Models/Transaction.h>

class TransactionValidator
{
public:
	TransactionValidator(BulletProofsCache& bulletproofsCache);

	bool ValidateTransaction(const Transaction& transaction);

private:
	bool ValidateFeatures(const TransactionBody& transactionBody);
	bool ValidateKernelSums(const Transaction& transaction);

	TransactionBodyValidator m_transactionBodyValidator;
};