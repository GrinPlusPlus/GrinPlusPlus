#include <Core/Validation/TransactionBodyValidator.h>

#include <Core/Validation/KernelSignatureValidator.h>
#include <Consensus/BlockWeight.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/Crypto.h>
#include <set>

// Validates all relevant parts of a transaction body. 
// Checks the excess value against the signature as well as range proofs for each output.
bool TransactionBodyValidator::ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward)
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

	if (!VerifyRangeProofs(transactionBody.GetOutputs()))
	{
		return false;
	}

	if (!KernelSignatureValidator::VerifyKernelSignatures(transactionBody.GetKernels()))
	{
		return false;
	}

	return true;
}

// Verify the body is not too big in terms of number of inputs|outputs|kernels.
bool TransactionBodyValidator::ValidateWeight(const TransactionBody& transactionBody, const bool withReward)
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

bool TransactionBodyValidator::VerifySorted(const TransactionBody& transactionBody)
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
bool TransactionBodyValidator::VerifyCutThrough(const TransactionBody& transactionBody)
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

bool TransactionBodyValidator::VerifyRangeProofs(const std::vector<TransactionOutput>& outputs)
{
	std::vector<std::pair<Commitment, RangeProof>> rangeProofs;
	for (auto output : outputs)
	{
		rangeProofs.emplace_back(std::make_pair<Commitment, RangeProof>(Commitment(output.GetCommitment()), RangeProof(output.GetRangeProof())));
	}

	return Crypto::VerifyRangeProofs(rangeProofs);
}