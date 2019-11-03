#include "ReceiveSlateBuilder.h"
#include "SignatureUtil.h"
#include "TransactionBuilder.h"

#include <Core/Exceptions/WalletException.h>
#include <Wallet/WalletUtil.h>
#include <Wallet/SlateContext.h>
#include <Infrastructure/Logger.h>

Slate ReceiveSlateBuilder::AddReceiverData(
	Locked<Wallet> wallet,
	const SecureVector& masterSeed,
	const Slate& slate,
	const std::optional<std::string>& addressOpt,
	const std::optional<std::string>& messageOpt) const
{
	auto pWallet = wallet.Write();
	Slate receiveSlate = slate;

	// Verify slate was never received, exactly one kernel exists, and fee is valid.
	if (!VerifySlateStatus(pWallet.GetShared(), masterSeed, receiveSlate))
	{
		throw WALLET_EXCEPTION("Failed to verify received slate.");
	}

	// Generate output
	const uint32_t walletTxId = pWallet->GetNextWalletTxId();
	OutputData outputData = pWallet->CreateBlindedOutput(masterSeed, receiveSlate.GetAmount(), walletTxId, EBulletproofType::ENHANCED, messageOpt);
	SecretKey secretKey = outputData.GetBlindingFactor();
	SecretKey secretNonce = *Crypto::GenerateSecureNonce();

	// Add receiver ParticipantData
	AddParticipantData(receiveSlate, secretKey, secretNonce, messageOpt);

	// Add output to Transaction
	receiveSlate.UpdateTransaction(TransactionBuilder::AddOutput(receiveSlate.GetTransaction(), outputData.GetOutput()));

	const SlateContext slateContext(std::move(secretKey), std::move(secretNonce));
	if (!UpdateDatabase(pWallet.GetShared(), masterSeed, receiveSlate, outputData, walletTxId, slateContext, addressOpt, messageOpt))
	{
		throw WALLET_EXCEPTION("Failed to update wallet database.");
	}

	return receiveSlate;
}

bool ReceiveSlateBuilder::VerifySlateStatus(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, const Slate& slate) const
{
	// Slate was already received.
	std::unique_ptr<WalletTx> pWalletTx = pWallet->GetTxBySlateId(masterSeed, slate.GetSlateId());
	if (pWalletTx != nullptr && pWalletTx->GetType() != EWalletTxType::RECEIVED_CANCELED)
	{
		WALLET_ERROR_F("Already received slate %s", uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Verify slate message signatures
	if (!SignatureUtil::VerifyMessageSignatures(slate.GetParticipantData()))
	{
		WALLET_ERROR_F("Failed to verify message signatures for slate %s", uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// TODO: Verify fees

	// Generate message
	const std::vector<TransactionKernel>& kernels = slate.GetTransaction().GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		WALLET_ERROR_F("Slate %s had %llu kernels.", uuids::to_string(slate.GetSlateId()), kernels.size());
		return false;
	}

	return true;
}

void ReceiveSlateBuilder::AddParticipantData(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce, const std::optional<std::string>& messageOpt) const
{
	const Hash kernelMessage = slate.GetTransaction().GetBody().GetKernels().front().GetSignatureMessage();

	std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(secretKey);
	std::unique_ptr<PublicKey> pPublicNonce = Crypto::CalculatePublicKey(secretNonce);
	if (pPublicKey == nullptr || pPublicNonce == nullptr)
	{
		throw CryptoException();
	}

	// Build receiver's ParticipantData
	ParticipantData receiverData(1, *pPublicKey, *pPublicNonce);

	// Generate signature
	std::vector<ParticipantData> participants = slate.GetParticipantData();
	participants.emplace_back(receiverData);

	std::unique_ptr<CompactSignature> pPartialSignature = SignatureUtil::GeneratePartialSignature(secretKey, secretNonce, participants, kernelMessage);
	if (pPartialSignature == nullptr)
	{
		WALLET_ERROR_F("Failed to generate signature for slate %s", uuids::to_string(slate.GetSlateId()));
		throw CryptoException();
	}

	receiverData.AddPartialSignature(*pPartialSignature);

	// Add message signature
	if (messageOpt.has_value())
	{
		// TODO: Limit message length
		std::unique_ptr<CompactSignature> pMessageSignature = Crypto::SignMessage(secretKey, *pPublicKey, messageOpt.value());
		if (pMessageSignature == nullptr)
		{
			WALLET_ERROR_F("Failed to sign message for slate %s", uuids::to_string(slate.GetSlateId()));
			throw CryptoException();
		}

		receiverData.AddMessage(messageOpt.value(), *pMessageSignature);
	}

	// Add receiver's ParticipantData to Slate
	slate.AddParticpantData(receiverData);

	if (!SignatureUtil::VerifyPartialSignatures(slate.GetParticipantData(), kernelMessage))
	{
		WALLET_ERROR_F("Failed to verify signature for slate %s", uuids::to_string(slate.GetSlateId()));
		throw CryptoException();
	}
}

// TODO: Use a DB Batch
bool ReceiveSlateBuilder::UpdateDatabase(
	std::shared_ptr<Wallet> pWallet,
	const SecureVector& masterSeed,
	const Slate& slate,
	const OutputData& outputData,
	const uint32_t walletTxId,
	const SlateContext& context,
	const std::optional<std::string>& addressOpt,
	const std::optional<std::string>& messageOpt) const
{
	// Save secretKey and secretNonce
	if (!pWallet->SaveSlateContext(slate.GetSlateId(), masterSeed, context))
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::UpdateDatabase - Failed to save context for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Save OutputData
	if (!pWallet->SaveOutputs(masterSeed, std::vector<OutputData>({ outputData })))
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::UpdateDatabase - Failed to save output for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Save WalletTx
	WalletTx walletTx(
		walletTxId,
		EWalletTxType::RECEIVING_IN_PROGRESS,
		std::make_optional<uuids::uuid>(uuids::uuid(slate.GetSlateId())),
		std::optional<std::string>(addressOpt),
		std::optional<std::string>(messageOpt),
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		slate.GetAmount(),
		0,
		std::optional<uint64_t>(slate.GetFee()),
		std::nullopt
	);

	if (!pWallet->AddWalletTxs(masterSeed, std::vector<WalletTx>({ walletTx })))
	{
		WALLET_ERROR("Failed to create WalletTx");
		return false;
	}

	return true;
}