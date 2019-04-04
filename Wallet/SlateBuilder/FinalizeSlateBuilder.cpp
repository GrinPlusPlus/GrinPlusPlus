#include "FinalizeSlateBuilder.h"
#include "TransactionBuilder.h"
#include "SignatureUtil.h"

#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/TransactionValidator.h>

std::unique_ptr<Slate> FinalizeSlateBuilder::Finalize(Wallet& wallet, const SecureVector& masterSeed, const Slate& slate) const
{
	Slate finalSlate = slate;

	// Verify transaction contains exactly one kernel
	const Transaction& transaction = finalSlate.GetTransaction();
	const std::vector<TransactionKernel>& kernels = transaction.GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	// Verify slate message signatures
	if (!SignatureUtil::VerifyMessageSignatures(slate.GetParticipantData()))
	{
		LoggerAPI::LogError("FinalizeSlateBuilder::Finalize - Failed to verify message signatures for slate " + uuids::to_string(slate.GetSlateId()));
		return std::unique_ptr<Slate>(nullptr);
	}

	// Verify partial signatures
	const Hash message = kernels.front().GetSignatureMessage();
	if (!SignatureUtil::VerifyPartialSignatures(finalSlate.GetParticipantData(), message))
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	// Add partial signature to slate's participant data
	if (!AddPartialSignature(wallet, masterSeed, finalSlate, message))
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	// Finalize transaction
	if (!AddFinalTransaction(finalSlate, message))
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	// Update database with latest WalletTx
	if (!UpdateDatabase(wallet, masterSeed, finalSlate))
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	return std::make_unique<Slate>(std::move(finalSlate));
}

bool FinalizeSlateBuilder::AddPartialSignature(Wallet& wallet, const SecureVector& masterSeed, Slate& slate, const Hash& kernelMessage) const
{
	// Load secretKey and secretNonce
	std::unique_ptr<SlateContext> pSlateContext = wallet.GetSlateContext(slate.GetSlateId(), masterSeed);
	if (pSlateContext == nullptr)
	{
		return false;
	}

	// Generate partial signature
	std::unique_ptr<Signature> pPartialSignature = SignatureUtil::GeneratePartialSignature(pSlateContext->GetSecretKey(), pSlateContext->GetSecretNonce(), slate.GetParticipantData(), kernelMessage);
	if (pPartialSignature == nullptr)
	{
		return false;
	}

	std::vector<ParticipantData>& participants = slate.GetParticipantData();
	for (ParticipantData& participant : participants)
	{
		if (participant.GetParticipantId() == 0)
		{
			participant.AddPartialSignature(*pPartialSignature);
			return true;
		}
	}

	return false;
}

bool FinalizeSlateBuilder::AddFinalTransaction(Slate& slate, const Hash& kernelMessage) const
{
	// Aggregate partial signatures
	std::unique_ptr<Signature> pAggSignature = SignatureUtil::AggregateSignatures(slate.GetParticipantData());
	if (pAggSignature == nullptr)
	{
		return false;
	}

	if (!SignatureUtil::VerifyAggregateSignature(*pAggSignature, slate.GetParticipantData(), kernelMessage))
	{
		return false;
	}

	const Transaction& transaction = slate.GetTransaction();
	const TransactionKernel& kernel = slate.GetTransaction().GetBody().GetKernels().front();

	// Build the final excess based on final tx and offset
	auto getInputCommitments = [](const TransactionInput& input) -> Commitment { return input.GetCommitment(); };
	std::vector<Commitment> inputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetInputs(), getInputCommitments);
	inputCommitments.emplace_back(*Crypto::CommitBlinded(0, transaction.GetOffset()));

	auto getOutputCommitments = [](const TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
	std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetOutputs(), getOutputCommitments);
	outputCommitments.emplace_back(*Crypto::CommitTransparent(kernel.GetFee()));

	std::unique_ptr<Commitment> pFinalExcess = Crypto::AddCommitments(outputCommitments, inputCommitments);

	// Update the tx kernel to reflect the offset excess and sig
	TransactionKernel finalKernel(kernel.GetFeatures(), kernel.GetFee(), kernel.GetLockHeight(), Commitment(*pFinalExcess), Signature(*pAggSignature));
	if (!KernelSignatureValidator().VerifyKernelSignatures(std::vector<TransactionKernel>({ finalKernel })))
	{
		return false;
	}

	const Transaction finalTransaction = TransactionBuilder::ReplaceKernel(slate.GetTransaction(), finalKernel);
	if (!TransactionValidator().ValidateTransaction(finalTransaction))
	{
		return false;
	}

	slate.UpdateTransaction(finalTransaction);

	return true;
}

bool FinalizeSlateBuilder::UpdateDatabase(Wallet& wallet, const SecureVector& masterSeed, Slate& finalSlate) const
{
	// Load WalletTx
	std::unique_ptr<WalletTx> pWalletTx = wallet.GetTxBySlateId(masterSeed, finalSlate.GetSlateId());
	if (pWalletTx == nullptr || pWalletTx->GetType() != EWalletTxType::SENDING_STARTED)
	{
		return false;
	}

	// Update WalletTx
	pWalletTx->SetType(EWalletTxType::SENDING_FINALIZED);
	pWalletTx->SetTransaction(finalSlate.GetTransaction());

	if (!wallet.AddWalletTxs(masterSeed, std::vector<WalletTx>({ *pWalletTx })))
	{
		return false;
	}

	return true;
}