#include "FinalizeSlateBuilder.h"
#include "TransactionBuilder.h"
#include "SignatureUtil.h"
#include "SlateUtil.h"

#include <Core/Exceptions/WalletException.h>
#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/TransactionValidator.h>

Slate FinalizeSlateBuilder::Finalize(
	Locked<Wallet> wallet,
	const SecureVector& masterSeed,
	const Slate& slate) const
{
	Slate finalSlate = slate;
	auto walletWriter = wallet.Write();

	// Verify transaction contains exactly one kernel
	const Transaction& transaction = finalSlate.GetTransaction();
	const std::vector<TransactionKernel>& kernels = transaction.GetKernels();
	if (kernels.size() != 1)
	{
		WALLET_ERROR_F(
			"Slate {} had {} kernels.",
			uuids::to_string(slate.GetSlateId()),
			kernels.size()
		);
		throw WALLET_EXCEPTION("Invalid number of kernels.");
	}

	// Verify slate message signatures
	if (!SignatureUtil::VerifyMessageSignatures(slate.GetParticipantData()))
	{
		WALLET_ERROR_F(
			"Failed to verify message signatures for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		throw WALLET_EXCEPTION("Failed to verify message signatures.");
	}

	// Verify partial signatures
	const Hash message = kernels.front().GetSignatureMessage();
	if (!SignatureUtil::VerifyPartialSignatures(finalSlate.GetParticipantData(), message))
	{
		WALLET_ERROR_F(
			"Failed to verify partial signatures for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		throw WALLET_EXCEPTION("Failed to verify partial signatures.");
	}

	// Load WalletTx
	std::unique_ptr<WalletTx> pWalletTx = walletWriter->GetTxBySlateId(
		masterSeed,
		finalSlate.GetSlateId()
	);
	if (pWalletTx == nullptr || pWalletTx->GetType() != EWalletTxType::SENDING_STARTED)
	{
		const std::string errorMsg = StringUtil::Format(
			"Transaction not found for slate ({})",
			uuids::to_string(finalSlate.GetSlateId())
		);
		WALLET_ERROR(errorMsg);
		throw WALLET_EXCEPTION(errorMsg);
	}

	Commitment finalExcess = SlateUtil::CalculateFinalExcess(slate);

	// Verify payment proof addresses & signatures
	if (!VerifyPaymentProofs(pWalletTx, finalSlate, finalExcess))
	{
		WALLET_ERROR_F(
			"Failed to verify payment proof for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		throw WALLET_EXCEPTION("Failed to verify payment proof.");
	}

	// Add partial signature to slate's participant data
	if (!AddPartialSignature(walletWriter.GetShared(), masterSeed, finalSlate, message))
	{
		WALLET_ERROR_F(
			"Failed to generate signatures for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		throw WALLET_EXCEPTION("Failed to generate signatures.");
	}

	// Finalize transaction
	if (!AddFinalTransaction(finalSlate, message, finalExcess))
	{
		WALLET_ERROR_F(
			"Failed to verify finalized transaction for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		throw WALLET_EXCEPTION("Failed to verify finalized transaction.");
	}

	// Update database with latest WalletTx
	UpdateDatabase(walletWriter.GetShared(), masterSeed, finalSlate);

	return finalSlate;
}

bool FinalizeSlateBuilder::AddPartialSignature(
	const Wallet::CPtr& pWallet,
	const SecureVector& masterSeed, Slate& slate,
	const Hash& kernelMessage) const
{
	// Load secretKey and secretNonce
	auto pSlateContext = pWallet->GetDatabase().Read()->LoadSlateContext(
		masterSeed,
		slate.GetSlateId()
	);
	if (pSlateContext == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to load slate context for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		return false;
	}

	// Generate partial signature
	std::unique_ptr<CompactSignature> pPartialSignature = SignatureUtil::GeneratePartialSignature(
		pSlateContext->GetSecretKey(),
		pSlateContext->GetSecretNonce(),
		slate.GetParticipantData(),
		kernelMessage
	);
	if (pPartialSignature == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to calculate partial signature for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
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

bool FinalizeSlateBuilder::AddFinalTransaction(
	Slate& slate,
	const Hash& kernelMessage,
	const Commitment& finalExcess) const
{
	// Aggregate partial signatures
	auto pAggSignature = SignatureUtil::AggregateSignatures(slate.GetParticipantData());
	if (pAggSignature == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to aggregate signatures for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		return false;
	}

	if (!SignatureUtil::VerifyAggregateSignature(*pAggSignature, slate.GetParticipantData(), kernelMessage))
	{
		WALLET_ERROR_F(
			"Failed to verify aggregate signature for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		return false;
	}

	const TransactionKernel& kernel = slate.GetTransaction().GetKernels().front();

	// Update the tx kernel to reflect the offset excess and sig
	TransactionKernel finalKernel(
		kernel.GetFeatures(),
		kernel.GetFee(),
		kernel.GetLockHeight(),
		Commitment(finalExcess),
		Signature(*pAggSignature)
	);
	if (!KernelSignatureValidator().VerifyKernelSignatures(std::vector<TransactionKernel>({ finalKernel })))
	{
		WALLET_ERROR_F(
			"Failed to verify kernel signatures for slate {}",
			uuids::to_string(slate.GetSlateId())
		);
		return false;
	}

	const Transaction finalTransaction = TransactionBuilder::ReplaceKernel(
		slate.GetTransaction(),
		finalKernel
	);
	try
	{
		TransactionValidator().Validate(finalTransaction); // TODO: Check if inputs unspent(txHashSet->Validate())?
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F(
			"Failed to validate transaction for slate {}. Error: {}",
			uuids::to_string(slate.GetSlateId()),
			e.what()
		);
		return false;
	}

	slate.UpdateTransaction(finalTransaction);

	return true;
}

bool FinalizeSlateBuilder::VerifyPaymentProofs(
	const std::unique_ptr<WalletTx>& pWalletTx,
	const Slate& slate,
	const Commitment& finalExcess) const
{
	auto& paymentProofOpt = pWalletTx->GetPaymentProof();
	if (paymentProofOpt.has_value())
	{
		if (!slate.GetPaymentProof().has_value())
		{
			return false;
		}

		auto& origProof = *paymentProofOpt;
		auto& newProof = *slate.GetPaymentProof();

		if (origProof.GetReceiverAddress().pubkey != newProof.GetReceiverAddress().pubkey
			|| origProof.GetSenderAddress().pubkey != newProof.GetSenderAddress().pubkey
			|| !newProof.GetReceiverSignature().has_value())
		{
			return false;
		}

		// Check signature - Message: (amount | kernel commitment | sender address)
		Serializer messageSerializer;
		messageSerializer.Append<uint64_t>(slate.GetAmount());
		finalExcess.Serialize(messageSerializer);
		messageSerializer.AppendBigInteger(CBigInteger<32>(newProof.GetSenderAddress().pubkey));

		return ED25519::VerifySignature(
			newProof.GetReceiverAddress(),
			*newProof.GetReceiverSignature(),
			messageSerializer.GetBytes()
		);
	}

	return true;
}

void FinalizeSlateBuilder::UpdateDatabase(
	const Wallet::Ptr& pWallet,
	const SecureVector& masterSeed,
	Slate& finalSlate) const
{
	// Load WalletTx
	std::unique_ptr<WalletTx> pWalletTx = pWallet->GetTxBySlateId(masterSeed, finalSlate.GetSlateId());
	if (pWalletTx == nullptr || pWalletTx->GetType() != EWalletTxType::SENDING_STARTED)
	{
		const std::string message = StringUtil::Format(
			"Transaction not found for slate ({})",
			uuids::to_string(finalSlate.GetSlateId())
		);
		WALLET_ERROR(message);
		throw WALLET_EXCEPTION(message);
	}

	// Update WalletTx
	pWalletTx->SetType(EWalletTxType::SENDING_FINALIZED);
	pWalletTx->SetTransaction(finalSlate.GetTransaction());

	pWallet->GetDatabase().Write()->AddTransaction(masterSeed, *pWalletTx);
}