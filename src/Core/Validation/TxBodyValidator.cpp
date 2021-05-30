#include <Core/Validation/TxBodyValidator.h>

#include <Consensus.h>
#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Exceptions/BadDataException.h>
#include <Common/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/Crypto.h>
#include <set>

// Validates all relevant parts of a transaction body. 
// Checks the excess value against the signature as well as range proofs for each output.
void TxBodyValidator::Validate(const TransactionBody& body)
{
	VerifySorted(body);
	VerifyCutThrough(body);
	VerifyRangeProofs(body.GetOutputs());
	
	if (!KernelSignatureValidator::BatchVerify(body.GetKernels())) {
		throw BAD_DATA_EXCEPTION(EBanReason::BadTransaction, "Kernel signatures invalid");
	}
}

void TxBodyValidator::VerifySorted(const TransactionBody& body)
{
	const bool sorted = Consensus::IsSorted(body.GetInputs())
		&& Consensus::IsSorted(body.GetOutputs())
		&& Consensus::IsSorted(body.GetKernels());
	if (!sorted) {
		throw BAD_DATA_EXCEPTION(EBanReason::BadTransaction, "Inputs, outputs, and/or kernels not sorted.");
	}

	for (size_t i = 1; i < body.GetInputs().size(); i++) {
		if (body.GetInput(i) == body.GetInput(i - 1)) {
			throw BAD_DATA_EXCEPTION(EBanReason::BadTransaction, "Duplicate inputs.");
		}
	}

	for (size_t i = 1; i < body.GetOutputs().size(); i++) {
		if (body.GetOutput(i) == body.GetOutput(i - 1)) {
			throw BAD_DATA_EXCEPTION(EBanReason::BadTransaction, "Duplicate outputs.");
		}
	}
}

// Verify that no input is spending an output from the same block.
void TxBodyValidator::VerifyCutThrough(const TransactionBody& body)
{
	// Create set with output commitments
	std::set<Commitment> output_commits;
	std::transform(
		body.GetOutputs().cbegin(), body.GetOutputs().cend(),
		std::inserter(output_commits, output_commits.end()),
		[](const TransactionOutput& output) { return output.GetCommitment(); }
	);

	// Verify none of the input commitments are in the commitments set
	const bool invalid = std::any_of(
		body.GetInputs().cbegin(), body.GetInputs().cend(),
		[&output_commits](const TransactionInput& input) {
			return output_commits.find(input.GetCommitment()) != output_commits.cend();
		}
	);
	if (invalid) {
		throw BAD_DATA_EXCEPTION(EBanReason::BadTransaction, "Cut-through not performed correctly.");
	}
}

void TxBodyValidator::VerifyRangeProofs(const std::vector<TransactionOutput>& outputs)
{
	std::vector<std::pair<Commitment, RangeProof>> rangeProofs;
	std::transform(
		outputs.cbegin(), outputs.cend(),
		std::back_inserter(rangeProofs),
		[](const TransactionOutput& output) { return std::make_pair(output.GetCommitment(), output.GetRangeProof()); }
	);
	if (!Crypto::VerifyRangeProofs(rangeProofs)) {
		throw BAD_DATA_EXCEPTION(EBanReason::BadTransaction, "Range proofs invalid.");
	}
}