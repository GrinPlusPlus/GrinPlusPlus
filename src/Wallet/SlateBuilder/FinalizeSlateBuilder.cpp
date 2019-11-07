#include "FinalizeSlateBuilder.h"
#include "TransactionBuilder.h"
#include "SignatureUtil.h"

#include <Core/Exceptions/WalletException.h>
#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/TransactionValidator.h>

Slate FinalizeSlateBuilder::Finalize(Locked<Wallet> wallet, const SecureVector& masterSeed, const Slate& slate) const
{
	Slate finalSlate = slate;

	// Verify transaction contains exactly one kernel
	const Transaction& transaction = finalSlate.GetTransaction();
	const std::vector<TransactionKernel>& kernels = transaction.GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		WALLET_ERROR_F("Slate %s had %llu kernels.", uuids::to_string(slate.GetSlateId()), kernels.size());
		throw WALLET_EXCEPTION("Invalid number of kernels.");
	}

	// Verify slate message signatures
	if (!SignatureUtil::VerifyMessageSignatures(slate.GetParticipantData()))
	{
		WALLET_ERROR_F("Failed to verify message signatures for slate %s", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to verify message signatures.");
	}

	auto pWallet = wallet.Write();

	// Verify partial signatures
	const Hash message = kernels.front().GetSignatureMessage();
	if (!SignatureUtil::VerifyPartialSignatures(finalSlate.GetParticipantData(), message))
	{
		WALLET_ERROR_F("Failed to verify partial signatures for slate %s", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to verify partial signatures.");
	}

	// Add partial signature to slate's participant data
	if (!AddPartialSignature(pWallet.GetShared(), masterSeed, finalSlate, message))
	{
		WALLET_ERROR_F("Failed to generate signatures for slate %s", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to generate signatures.");
	}

	// Finalize transaction
	if (!AddFinalTransaction(finalSlate, message))
	{
		WALLET_ERROR_F("Failed to verify finalized transaction for slate %s", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to verify finalized transaction.");
	}

	// Update database with latest WalletTx
	if (!UpdateDatabase(pWallet.GetShared(), masterSeed, finalSlate))
	{
		WALLET_ERROR_F("Failed to update database for slate %s", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to update wallet database.");
	}

	return finalSlate;
}

bool FinalizeSlateBuilder::AddPartialSignature(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, Slate& slate, const Hash& kernelMessage) const
{
	// Load secretKey and secretNonce
	std::unique_ptr<SlateContext> pSlateContext = pWallet->GetSlateContext(slate.GetSlateId(), masterSeed);
	if (pSlateContext == nullptr)
	{
		return false;
	}

	// Generate partial signature
	std::unique_ptr<CompactSignature> pPartialSignature = SignatureUtil::GeneratePartialSignature(pSlateContext->GetSecretKey(), pSlateContext->GetSecretNonce(), slate.GetParticipantData(), kernelMessage);
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
	inputCommitments.emplace_back(Crypto::CommitBlinded(0, transaction.GetOffset()));

	auto getOutputCommitments = [](const TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
	std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetOutputs(), getOutputCommitments);
	outputCommitments.emplace_back(Crypto::CommitTransparent(kernel.GetFee()));

	Commitment finalExcess = Crypto::AddCommitments(outputCommitments, inputCommitments);

	// Update the tx kernel to reflect the offset excess and sig
	TransactionKernel finalKernel(
		kernel.GetFeatures(),
		kernel.GetFee(),
		kernel.GetLockHeight(),
		std::move(finalExcess),
		Signature(*pAggSignature)
	);
	if (!KernelSignatureValidator().VerifyKernelSignatures(std::vector<TransactionKernel>({ finalKernel })))
	{
		return false;
	}

	const Transaction finalTransaction = TransactionBuilder::ReplaceKernel(slate.GetTransaction(), finalKernel);
	try
	{
		TransactionValidator().Validate(finalTransaction); // TODO: Check if inputs unspent(txHashSet->Validate())?
	}
	catch (std::exception& e)
	{
		return false;
	}

	slate.UpdateTransaction(finalTransaction);

	return true;
}

bool FinalizeSlateBuilder::UpdateDatabase(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, Slate& finalSlate) const
{
	// Load WalletTx
	std::unique_ptr<WalletTx> pWalletTx = pWallet->GetTxBySlateId(masterSeed, finalSlate.GetSlateId());
	if (pWalletTx == nullptr || pWalletTx->GetType() != EWalletTxType::SENDING_STARTED)
	{
		return false;
	}

	// Update WalletTx
	pWalletTx->SetType(EWalletTxType::SENDING_FINALIZED);
	pWalletTx->SetTransaction(finalSlate.GetTransaction());

	if (!pWallet->AddWalletTxs(masterSeed, std::vector<WalletTx>({ *pWalletTx })))
	{
		return false;
	}

	return true;
}