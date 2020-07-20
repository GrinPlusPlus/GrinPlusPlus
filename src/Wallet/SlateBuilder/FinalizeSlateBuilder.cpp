#include "FinalizeSlateBuilder.h"
#include "TransactionBuilder.h"
#include "SignatureUtil.h"
#include "SlateUtil.h"

#include <Core/Exceptions/WalletException.h>
#include <Core/Validation/KernelSignatureValidator.h>
#include <Core/Validation/TransactionValidator.h>

// TODO: To help prevent "play" attacks, make sure any outputs provided by the receiver are not already on-chain.

std::pair<Slate, Transaction> FinalizeSlateBuilder::Finalize(
	Locked<Wallet> wallet,
	const SecureVector& masterSeed,
	const Slate& rcvSlate) const
{
	Slate finalizeSlate = rcvSlate;
	finalizeSlate.stage = ESlateStage::STANDARD_FINALIZED;
	auto walletWriter = wallet.Write();

	// Load original send Slate
	std::unique_ptr<Slate> pSendSlate = walletWriter->GetDatabase().Read()->LoadSlate(
		masterSeed,
		finalizeSlate.GetId(),
		ESlateStage::STANDARD_SENT
	);
	if (pSendSlate == nullptr)
	{
		const std::string errorMsg = StringUtil::Format(
			"Send slate not found for slate ({})",
			uuids::to_string(finalizeSlate.GetId())
		);
		WALLET_ERROR(errorMsg);
		throw WALLET_EXCEPTION(errorMsg);
	}

	// Load WalletTx
	std::unique_ptr<WalletTx> pWalletTx = walletWriter->GetTxBySlateId(
		masterSeed,
		finalizeSlate.GetId()
	);
	if (pWalletTx == nullptr || pWalletTx->GetType() != EWalletTxType::SENDING_STARTED)
	{
		const std::string errorMsg = StringUtil::Format(
			"Transaction not found for slate ({})",
			uuids::to_string(finalizeSlate.GetId())
		);
		WALLET_ERROR(errorMsg);
		throw WALLET_EXCEPTION(errorMsg);
	}

	// Load private context
	auto pSlateContext = walletWriter->GetDatabase().Read()->LoadSlateContext(
		masterSeed,
		finalizeSlate.GetId()
	);
	if (pSlateContext == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to load slate context for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		throw WALLET_EXCEPTION("Failed to load slate context.");
	}

	// Add inputs, outputs, and omitted fields
	finalizeSlate.amount = pSendSlate->amount;
	finalizeSlate.fee = pSendSlate->fee;
	finalizeSlate.kernelFeatures = pSendSlate->kernelFeatures;
	finalizeSlate.featureArgsOpt = pSendSlate->featureArgsOpt;
	finalizeSlate.ttl = pSendSlate->ttl;
	std::for_each(
		pSlateContext->GetInputs().cbegin(), pSlateContext->GetInputs().cend(),
		[&finalizeSlate](const OutputDataEntity& input) {
			finalizeSlate.commitments.push_back(SlateCommitment{ input.GetFeatures(), input.GetCommitment(), std::nullopt });
		}
	);

	std::for_each(
		pSlateContext->GetOutputs().cbegin(), pSlateContext->GetOutputs().cend(),
		[&finalizeSlate](const OutputDataEntity& output) {
			finalizeSlate.commitments.push_back(SlateCommitment{ output.GetFeatures(), output.GetCommitment(), output.GetRangeProof() });
		}
	);
	Commitment finalExcess = SlateUtil::CalculateFinalExcess(finalizeSlate);

	// Verify payment proof addresses & signatures
	if (!VerifyPaymentProofs(pWalletTx, finalizeSlate, finalExcess))
	{
		WALLET_ERROR_F(
			"Failed to verify payment proof for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		throw WALLET_EXCEPTION("Failed to verify payment proof.");
	}

	const Hash kernelMessage = TransactionKernel::GetSignatureMessage(
		finalizeSlate.kernelFeatures,
		finalizeSlate.fee,
		finalizeSlate.GetLockHeight()
	);

	// Add partial signature to slate's participant data
	if (!AddPartialSignature(*pSlateContext, finalizeSlate, kernelMessage))
	{
		WALLET_ERROR_F(
			"Failed to generate signatures for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		throw WALLET_EXCEPTION("Failed to generate signatures.");
	}

	// Finalize transaction
	const std::unique_ptr<Transaction> pTransaction = BuildTransaction(finalizeSlate, kernelMessage, finalExcess);
	if (pTransaction == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to verify finalized transaction for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		throw WALLET_EXCEPTION("Failed to verify finalized transaction.");
	}

	// Update WalletTx
	pWalletTx->SetType(EWalletTxType::SENDING_FINALIZED);
	pWalletTx->SetTransaction(*pTransaction);

	// Update database with latest WalletTx
	UpdateDatabase(walletWriter.GetShared(), masterSeed, *pWalletTx, finalizeSlate);

	return std::make_pair<Slate, Transaction>(Slate(finalizeSlate), Transaction(*pTransaction));
}

bool FinalizeSlateBuilder::AddPartialSignature(
	const SlateContextEntity& context,
	Slate& finalizeSlate,
	const Hash& kernelMessage) const
{
	// Copy sig data from original send slate to finalize slate
	SlateSignature senderSig(
		context.CalcExcess(),
		context.CalcNonce(),
		std::nullopt
	);
	finalizeSlate.sigs.push_back(senderSig);

	// Generate partial signature
	std::unique_ptr<CompactSignature> pPartialSignature = SignatureUtil::GeneratePartialSignature(
		context.GetSecretKey(),
		context.GetSecretNonce(),
		finalizeSlate.sigs,
		kernelMessage
	);
	if (pPartialSignature == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to calculate partial signature for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		return false;
	}

	finalizeSlate.sigs.back().partialOpt = std::make_optional<CompactSignature>(*pPartialSignature);

	return true;
}

std::unique_ptr<Transaction> FinalizeSlateBuilder::BuildTransaction(
	Slate& finalizeSlate,
	const Hash& kernelMessage,
	const Commitment& finalExcess) const
{
	// Aggregate partial signatures
	auto pAggSignature = SignatureUtil::AggregateSignatures(finalizeSlate.sigs);
	if (pAggSignature == nullptr)
	{
		WALLET_ERROR_F(
			"Failed to aggregate signatures for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		return nullptr;
	}

	if (!SignatureUtil::VerifyAggregateSignature(*pAggSignature, finalizeSlate.sigs, kernelMessage))
	{
		WALLET_ERROR_F(
			"Failed to verify aggregate signature for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		return nullptr;
	}

	// Update the tx kernel to reflect the offset excess and sig
	TransactionKernel kernel(
		finalizeSlate.kernelFeatures,
		finalizeSlate.fee,
		finalizeSlate.GetLockHeight(),
		finalExcess,
		*pAggSignature
	);
	if (!KernelSignatureValidator().VerifyKernelSignatures(std::vector<TransactionKernel>({ kernel })))
	{
		WALLET_ERROR_F(
			"Failed to verify kernel signatures for slate {}",
			uuids::to_string(finalizeSlate.GetId())
		);
		return nullptr;
	}

	Transaction transaction = TransactionBuilder::BuildTransaction(
		finalizeSlate.GetInputs(),
		finalizeSlate.GetOutputs(),
		kernel,
		finalizeSlate.offset
	);
	try
	{
		TransactionValidator().Validate(transaction); // TODO: Check if inputs unspent(txHashSet->Validate())?
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F(
			"Failed to validate transaction for slate {}. Error: {}",
			uuids::to_string(finalizeSlate.GetId()),
			e.what()
		);
		return nullptr;
	}

	return std::make_unique<Transaction>(std::move(transaction));
}

bool FinalizeSlateBuilder::VerifyPaymentProofs(
	const std::unique_ptr<WalletTx>& pWalletTx,
	const Slate& finalizeSlate,
	const Commitment& finalExcess) const
{
	auto& paymentProofOpt = pWalletTx->GetPaymentProof();
	if (paymentProofOpt.has_value())
	{
		if (!finalizeSlate.GetPaymentProof().has_value())
		{
			return false;
		}

		auto& origProof = paymentProofOpt.value();
		auto& newProof = finalizeSlate.GetPaymentProof().value();

		if (origProof.GetReceiverAddress() != newProof.GetReceiverAddress()
			|| origProof.GetSenderAddress() != newProof.GetSenderAddress()
			|| !newProof.GetReceiverSignature().has_value())
		{
			return false;
		}

		// Check signature - Message: (amount | kernel commitment | sender address)
		Serializer messageSerializer;
		messageSerializer.Append<uint64_t>(finalizeSlate.GetAmount());
		finalExcess.Serialize(messageSerializer);
		messageSerializer.AppendBigInteger(newProof.GetSenderAddress().bytes);

		return ED25519::VerifySignature(
			newProof.GetReceiverAddress(),
			newProof.GetReceiverSignature().value(),
			messageSerializer.GetBytes()
		);
	}

	return true;
}

void FinalizeSlateBuilder::UpdateDatabase(
	const Wallet::Ptr& pWallet,
	const SecureVector& masterSeed,
	const WalletTx& walletTx,
	const Slate& finalizeSlate) const
{
	auto pBatch = pWallet->GetDatabase().BatchWrite();
	pBatch->SaveSlate(masterSeed, finalizeSlate);
	pBatch->AddTransaction(masterSeed, walletTx);
	pBatch->Commit();
}