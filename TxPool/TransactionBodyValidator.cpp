#include "TransactionBodyValidator.h"

#include <Consensus/BlockWeight.h>
#include <Infrastructure/Logger.h>
#include <HexUtil.h>
#include <Crypto.h>
#include <set>

// Validates all relevant parts of a transaction body. 
// Checks the excess value against the signature as well as range proofs for each output.
bool TransactionBodyValidator::ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const
{
	if (!ValidateWeight(transactionBody, withReward))
	{
		return false;
	}

	if (!VerifySorted(transactionBody))
	{
		return false;
	}

	if (!VerifyCutThrough(transactionBody))
	{
		return false;
	}

	if (!VerifyOutputs(transactionBody.GetOutputs()))
	{
		return false;
	}

	if (!VerifyKernels(transactionBody.GetKernels()))
	{
		return false;
	}

	return true;
}

// Verify the body is not too big in terms of number of inputs|outputs|kernels.
bool TransactionBodyValidator::ValidateWeight(const TransactionBody& transactionBody, const bool withReward) const
{
	// If with reward check the body as if it was a block, with an additional output and kernel for reward.
	const uint32_t reserve = withReward ? 1 : 0;
	const uint32_t blockInputWeight = ((uint32_t)transactionBody.GetInputs().size() * Consensus::BLOCK_INPUT_WEIGHT);
	const uint32_t blockOutputWeight = (((uint32_t)transactionBody.GetOutputs().size() + reserve) * Consensus::BLOCK_OUTPUT_WEIGHT);
	const uint32_t blockKernelWeight = (((uint32_t)transactionBody.GetKernels().size() + reserve) * Consensus::BLOCK_KERNEL_WEIGHT);

	if ((blockInputWeight + blockOutputWeight + blockKernelWeight) > Consensus::MAX_BLOCK_WEIGHT)
	{
		return false;
	}

	return true;
}

bool TransactionBodyValidator::VerifySorted(const TransactionBody& transactionBody) const
{
	for (auto iter = transactionBody.GetInputs().cbegin(); iter != transactionBody.GetInputs().cend(); iter++)
	{
		if (iter + 1 == transactionBody.GetInputs().cend())
		{
			break;
		}

		if (iter->GetHash() > (iter + 1)->GetHash())
		{
			return false;
		}
	}

	for (auto iter = transactionBody.GetOutputs().cbegin(); iter != transactionBody.GetOutputs().cend(); iter++)
	{
		if (iter + 1 == transactionBody.GetOutputs().cend())
		{
			break;
		}

		if (iter->GetHash() > (iter + 1)->GetHash())
		{
			return false;
		}
	}

	for (auto iter = transactionBody.GetKernels().cbegin(); iter != transactionBody.GetKernels().cend(); iter++)
	{
		if (iter + 1 == transactionBody.GetKernels().cend())
		{
			break;
		}

		if (iter->GetHash() > (iter + 1)->GetHash())
		{
			return false;
		}
	}

	return true;
}

// Verify that no input is spending an output from the same block.
bool TransactionBodyValidator::VerifyCutThrough(const TransactionBody& transactionBody) const
{
	std::set<Commitment> commitments;
	for (auto output : transactionBody.GetOutputs())
	{
		commitments.insert(output.GetCommitment());
	}

	for (auto input : transactionBody.GetInputs())
	{
		if (commitments.count(input.GetCommitment()) > 0)
		{
			return false;
		}
	}

	return true;
}

bool TransactionBodyValidator::VerifyOutputs(const std::vector<TransactionOutput>& outputs) const
{
	std::vector<Commitment> commitments;
	std::vector<RangeProof> proofs;

	for (auto output : outputs)
	{
		commitments.push_back(output.GetCommitment());
		proofs.push_back(output.GetRangeProof());
	}

	return Crypto::VerifyRangeProofs(commitments, proofs);
}

// Verify the unverified tx kernels.
// No ability to batch verify these right now
// so just do them individually.
bool TransactionBodyValidator::VerifyKernels(const std::vector<TransactionKernel>& kernels) const
{
	// Verify the transaction proof validity. Entails handling the commitment as a public key and checking the signature verifies with the fee as message.
	for (auto kernel : kernels)
	{
		const Commitment& publicKey = kernel.GetExcessCommitment();
		const Signature& signature = kernel.GetExcessSignature();
		const Hash signatureMessage = kernel.GetSignatureMessage();

		if (!Crypto::VerifyKernelSignature(signature, publicKey, signatureMessage))
		{
			LoggerAPI::LogError("TransactionBodyValidator::VerifyKernels - Failed to verify kernel " + HexUtil::ConvertHash(kernel.GetHash()));
			return false;
		}
	}

	return true;
}