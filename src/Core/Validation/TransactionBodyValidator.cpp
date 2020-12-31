#include <Core/Validation/TransactionBodyValidator.h>

#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Exceptions/BadDataException.h>
#include <Consensus/BlockWeight.h>
#include <Consensus/Sorting.h>
#include <Common/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/Crypto.h>
#include <set>

// Validates all relevant parts of a transaction body. 
// Checks the excess value against the signature as well as range proofs for each output.
void TransactionBodyValidator::Validate(const TransactionBody& body)
{
	VerifySorted(body);
	VerifyCutThrough(body);
	VerifyRangeProofs(body.GetOutputs());
	
	if (!KernelSignatureValidator::VerifyKernelSignatures(body.GetKernels()))
	{
		throw BAD_DATA_EXCEPTION("Kernel signatures invalid");
	}
}

void TransactionBodyValidator::VerifySorted(const TransactionBody& body)
{
	// TODO: Check for duplicates?
	const bool sorted = Consensus::IsSorted(body.GetInputs())
		&& Consensus::IsSorted(body.GetOutputs())
		&& Consensus::IsSorted(body.GetKernels());
	if (!sorted)
	{
		throw BAD_DATA_EXCEPTION("Inputs, outputs, and/or kernels not sorted.");
	}
}

// Verify that no input is spending an output from the same block.
void TransactionBodyValidator::VerifyCutThrough(const TransactionBody& body)
{
	const std::vector<TransactionOutput>& outputs = body.GetOutputs();
	const std::vector<TransactionInput>& inputs = body.GetInputs();

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