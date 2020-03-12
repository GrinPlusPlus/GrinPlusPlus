#include <Core/Validation/TransactionBodyValidator.h>

#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Exceptions/BadDataException.h>
#include <Consensus/BlockWeight.h>
#include <Consensus/Sorting.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/Crypto.h>
#include <set>

// Validates all relevant parts of a transaction body. 
// Checks the excess value against the signature as well as range proofs for each output.
void TransactionBodyValidator::Validate(const TransactionBody& transactionBody, const bool withReward)
{
	ValidateWeight(transactionBody, withReward);
	VerifySorted(transactionBody);
	VerifyCutThrough(transactionBody);
	VerifyRangeProofs(transactionBody.GetOutputs());
	
	if (!KernelSignatureValidator::VerifyKernelSignatures(transactionBody.GetKernels()))
	{
		throw BAD_DATA_EXCEPTION("Kernel signatures invalid");
	}
}

// Verify the body is not too big in terms of number of inputs|outputs|kernels.
void TransactionBodyValidator::ValidateWeight(const TransactionBody& transactionBody, const bool withReward)
{
	// If with reward check the body as if it was a block, with an additional output and kernel for reward.
	const uint32_t reserve = withReward ? 1 : 0;
	const uint32_t blockInputWeight = ((uint32_t)transactionBody.GetInputs().size() * Consensus::BLOCK_INPUT_WEIGHT);
	const uint32_t blockOutputWeight = (((uint32_t)transactionBody.GetOutputs().size() + reserve) * Consensus::BLOCK_OUTPUT_WEIGHT);
	const uint32_t blockKernelWeight = (((uint32_t)transactionBody.GetKernels().size() + reserve) * Consensus::BLOCK_KERNEL_WEIGHT);

	if ((blockInputWeight + blockOutputWeight + blockKernelWeight) > Consensus::MAX_BLOCK_WEIGHT)
	{
		throw BAD_DATA_EXCEPTION("Block weight invalid");
	}
}

void TransactionBodyValidator::VerifySorted(const TransactionBody& transactionBody)
{
	// TODO: Check for duplicates?
	const bool sorted = Consensus::IsSorted(transactionBody.GetInputs())
		&& Consensus::IsSorted(transactionBody.GetOutputs())
		&& Consensus::IsSorted(transactionBody.GetKernels());
	if (!sorted)
	{
		throw BAD_DATA_EXCEPTION("Inputs, outputs, and/or kernels not sorted.");
	}
}

// Verify that no input is spending an output from the same block.
void TransactionBodyValidator::VerifyCutThrough(const TransactionBody& transactionBody)
{
	const std::vector<TransactionOutput>& outputs = transactionBody.GetOutputs();
	const std::vector<TransactionInput>& inputs = transactionBody.GetInputs();

	// Create set with output commitments
	std::set<Commitment> commitments;
	std::transform(
		outputs.cbegin(),
		outputs.cend(),
		std::inserter(commitments, commitments.end()),
		[](const TransactionOutput& output) { return output.GetCommitment(); }
	);

	// Verify none of the input commitments are in the commitments set
	const bool invalid = std::any_of(
		inputs.cbegin(),
		inputs.cend(),
		[&commitments](const TransactionInput& input) { return commitments.find(input.GetCommitment()) != commitments.cend(); }
	);
	if (invalid)
	{
		throw BAD_DATA_EXCEPTION("Cut-through not performed correctly.");
	}
}

void TransactionBodyValidator::VerifyRangeProofs(const std::vector<TransactionOutput>& outputs)
{
	std::vector<std::pair<Commitment, RangeProof>> rangeProofs;
	std::transform(
		outputs.cbegin(),
		outputs.cend(),
		std::back_inserter(rangeProofs),
		[](const TransactionOutput& output) { return std::make_pair(output.GetCommitment(), output.GetRangeProof()); }
	);

	if (!Crypto::VerifyRangeProofs(rangeProofs))
	{
		throw BAD_DATA_EXCEPTION("Range proofs invalid.");
	}
}