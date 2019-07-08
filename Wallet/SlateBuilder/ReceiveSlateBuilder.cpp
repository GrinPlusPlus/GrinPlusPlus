#include "ReceiveSlateBuilder.h"
#include "SignatureUtil.h"
#include "TransactionBuilder.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/SlateContext.h>
#include <Infrastructure/Logger.h>

std::unique_ptr<Slate> ReceiveSlateBuilder::AddReceiverData(Wallet& wallet, const SecureVector& masterSeed, const Slate& slate, const std::optional<std::string>& messageOpt) const
{
	Slate receiveSlate = slate;

	// Verify slate was never received, exactly one kernel exists, and fee is valid.
	if (!VerifySlateStatus(wallet, masterSeed, receiveSlate))
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	// Generate output
	const uint32_t walletTxId = wallet.GetNextWalletTxId();
	OutputData outputData = wallet.CreateBlindedOutput(masterSeed, receiveSlate.GetAmount(), walletTxId, EBulletproofType::ENHANCED);
	SecretKey secretKey = outputData.GetBlindingFactor();
	SecretKey secretNonce = *Crypto::GenerateSecureNonce();

	// Add receiver ParticipantData
	AddParticipantData(receiveSlate, secretKey, secretNonce, messageOpt);

	// Add output to Transaction
	receiveSlate.UpdateTransaction(TransactionBuilder::AddOutput(receiveSlate.GetTransaction(), outputData.GetOutput()));

	const SlateContext slateContext(std::move(secretKey), std::move(secretNonce));
	if (!UpdateDatabase(wallet, masterSeed, receiveSlate, outputData, walletTxId, slateContext, messageOpt))
	{
		return std::unique_ptr<Slate>(nullptr);
	}

	return std::make_unique<Slate>(std::move(receiveSlate));
}

bool ReceiveSlateBuilder::VerifySlateStatus(Wallet& wallet, const SecureVector& masterSeed, const Slate& slate) const
{
	// Slate was already received.
	std::unique_ptr<WalletTx> pWalletTx = wallet.GetTxBySlateId(masterSeed, slate.GetSlateId());
	if (pWalletTx != nullptr && pWalletTx->GetType() != EWalletTxType::RECEIVED_CANCELED)
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::VerifySlateStatus - Already received slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Verify slate message signatures
	if (!SignatureUtil::VerifyMessageSignatures(slate.GetParticipantData()))
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::VerifySlateStatus - Failed to verify message signatures for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// TODO: Verify fees

	// Generate message
	const std::vector<TransactionKernel>& kernels = slate.GetTransaction().GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::VerifySlateStatus - Slate " + uuids::to_string(slate.GetSlateId()) + " had " + std::to_string(kernels.size()) + " kernels.");
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

	std::unique_ptr<Signature> pPartialSignature = SignatureUtil::GeneratePartialSignature(secretKey, secretNonce, participants, kernelMessage);
	if (pPartialSignature == nullptr)
	{
		LoggerAPI::LogError("SlateBuilder::AddParticipantData - Failed to generate signature for slate " + uuids::to_string(slate.GetSlateId()));
		throw CryptoException();
	}

	receiverData.AddPartialSignature(*pPartialSignature);

	// Add message signature
	if (messageOpt.has_value())
	{
		const Hash message = Crypto::Blake2b(std::vector<unsigned char>(messageOpt.value().cbegin(), messageOpt.value().cend()));
		std::unique_ptr<Signature> pMessageSignature = Crypto::SignMessage(secretKey, *pPublicKey, message);
		if (pMessageSignature == nullptr)
		{
			LoggerAPI::LogError("SlateBuilder::AddParticipantData - Failed to sign message for slate " + uuids::to_string(slate.GetSlateId()));
			throw CryptoException();
		}

		receiverData.AddMessage(messageOpt.value(), *pMessageSignature);
	}

	// Add receiver's ParticipantData to Slate
	slate.AddParticpantData(receiverData);

	if (!SignatureUtil::VerifyPartialSignatures(slate.GetParticipantData(), kernelMessage))
	{
		LoggerAPI::LogError("SlateBuilder::AddParticipantData - Failed to verify signature for slate " + uuids::to_string(slate.GetSlateId()));
		throw CryptoException();
	}
}

// TODO: Use a DB Batch
bool ReceiveSlateBuilder::UpdateDatabase(
	Wallet& wallet, 
	const SecureVector& masterSeed, 
	const Slate& slate, 
	const OutputData& outputData, 
	const uint32_t walletTxId, 
	const SlateContext& context, 
	const std::optional<std::string>& messageOpt) const
{
	// Save secretKey and secretNonce
	if (!wallet.SaveSlateContext(slate.GetSlateId(), masterSeed, context))
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::UpdateDatabase - Failed to save context for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Save OutputData
	if (!wallet.SaveOutputs(masterSeed, std::vector<OutputData>({ outputData })))
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::UpdateDatabase - Failed to save output for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Save WalletTx
	WalletTx walletTx(
		walletTxId,
		EWalletTxType::RECEIVING_IN_PROGRESS,
		std::make_optional<uuids::uuid>(uuids::uuid(slate.GetSlateId())),
		std::optional<std::string>(messageOpt),
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		slate.GetAmount(),
		0,
		std::optional<uint64_t>(slate.GetFee()),
		std::nullopt
	);

	if (!wallet.AddWalletTxs(masterSeed, std::vector<WalletTx>({ walletTx })))
	{
		LoggerAPI::LogError("ReceiveSlateBuilder::UpdateDatabase - Failed to create WalletTx");
		return false;
	}

	return true;
}