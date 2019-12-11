#include "ReceiveSlateBuilder.h"
#include "SignatureUtil.h"
#include "TransactionBuilder.h"
#include "SlateUtil.h"

#include <Core/Exceptions/WalletException.h>
#include <Wallet/WalletUtil.h>
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/Models/SlateContextEntity.h>

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

	auto pBatch = pWallet->GetDatabase().BatchWrite();

	// Generate output
	KeyChainPath keyChainPath = pBatch->GetNextChildPath(pWallet->GetUserPath());
	const uint32_t walletTxId = pBatch->GetNextTransactionId();
	OutputDataEntity outputData = pWallet->CreateBlindedOutput(masterSeed, receiveSlate.GetAmount(), keyChainPath, walletTxId, EBulletproofType::ENHANCED, messageOpt);
	SecretKey secretKey = outputData.GetBlindingFactor();
	SecretKey secretNonce = Crypto::GenerateSecureNonce();

	// Add receiver ParticipantData
	AddParticipantData(receiveSlate, secretKey, secretNonce, messageOpt);

	// Add output to Transaction
	receiveSlate.UpdateTransaction(TransactionBuilder::AddOutput(receiveSlate.GetTransaction(), outputData.GetOutput()));

	UpdatePaymentProof(pWallet.GetShared(), pBatch.GetShared(), masterSeed, receiveSlate);

	UpdateDatabase(pBatch, masterSeed, receiveSlate, outputData, walletTxId, addressOpt, messageOpt);

	pBatch->Commit();

	return receiveSlate;
}

bool ReceiveSlateBuilder::VerifySlateStatus(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, const Slate& slate) const
{
	// Slate was already received.
	std::unique_ptr<WalletTx> pWalletTx = pWallet->GetTxBySlateId(masterSeed, slate.GetSlateId());
	if (pWalletTx != nullptr && pWalletTx->GetType() != EWalletTxType::RECEIVED_CANCELED)
	{
		WALLET_ERROR_F("Already received slate {}", uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// Verify slate message signatures
	if (!SignatureUtil::VerifyMessageSignatures(slate.GetParticipantData()))
	{
		WALLET_ERROR_F("Failed to verify message signatures for slate {}", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to verify message signatures.");
	}

	// TODO: Verify fees

	// Generate message
	const std::vector<TransactionKernel>& kernels = slate.GetTransaction().GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		WALLET_ERROR_F("Slate {} had {} kernels.", uuids::to_string(slate.GetSlateId()), kernels.size());
		throw WALLET_EXCEPTION("Expected 1 kernel.");
	}

	return true;
}

void ReceiveSlateBuilder::AddParticipantData(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce, const std::optional<std::string>& messageOpt) const
{
	const Hash kernelMessage = slate.GetTransaction().GetBody().GetKernels().front().GetSignatureMessage();

	PublicKey publicKey = Crypto::CalculatePublicKey(secretKey);
	PublicKey publicNonce = Crypto::CalculatePublicKey(secretNonce);

	// Build receiver's ParticipantData
	ParticipantData receiverData(1, publicKey, publicNonce);

	// Generate signature
	std::vector<ParticipantData> participants = slate.GetParticipantData();
	participants.emplace_back(receiverData);

	std::unique_ptr<CompactSignature> pPartialSignature = SignatureUtil::GeneratePartialSignature(secretKey, secretNonce, participants, kernelMessage);
	if (pPartialSignature == nullptr)
	{
		WALLET_ERROR_F("Failed to generate signature for slate {}", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to generate signature.");
	}

	receiverData.AddPartialSignature(*pPartialSignature);

	// Add message signature
	if (messageOpt.has_value())
	{
		// TODO: Limit message length
		std::unique_ptr<CompactSignature> pMessageSignature = Crypto::SignMessage(secretKey, publicKey, messageOpt.value());
		if (pMessageSignature == nullptr)
		{
			WALLET_ERROR_F("Failed to sign message for slate {}", uuids::to_string(slate.GetSlateId()));
			throw WALLET_EXCEPTION("Failed to sign slate message.");
		}

		receiverData.AddMessage(messageOpt.value(), *pMessageSignature);
	}

	// Add receiver's ParticipantData to Slate
	slate.AddParticpantData(receiverData);

	if (!SignatureUtil::VerifyPartialSignatures(slate.GetParticipantData(), kernelMessage))
	{
		WALLET_ERROR_F("Failed to verify signature for slate {}", uuids::to_string(slate.GetSlateId()));
		throw WALLET_EXCEPTION("Failed to verify signatures.");
	}
}

void ReceiveSlateBuilder::UpdatePaymentProof(std::shared_ptr<Wallet> pWallet, IWalletDBPtr pWalletDB, const SecureVector& masterSeed, Slate& slate) const
{
	if (slate.GetPaymentProof().has_value())
	{
		if (!pWallet->GetTorAddress().has_value())
		{
			throw WALLET_EXCEPTION("");
		}

		auto& proof = slate.GetPaymentProof().value();
		if (proof.GetReceiverAddress() != pWallet->GetTorAddress().value().GetPublicKey())
		{
			throw WALLET_EXCEPTION("");
		}

		// Message: (amount | kernel commitment | sender address)
		Serializer messageSerializer;
		messageSerializer.Append<uint64_t>(slate.GetAmount());
		SlateUtil::CalculateFinalExcess(slate).Serialize(messageSerializer);
		messageSerializer.AppendBigInteger(CBigInteger<32>(proof.GetSenderAddress().pubkey));

		KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);
		SecretKey64 torKey = keyChain.DeriveED25519Key(KeyChainPath::FromString("m/0/1/0"));

		Signature signature = ED25519::Sign(torKey, proof.GetReceiverAddress(), messageSerializer.GetBytes());
		proof.AddSignature(std::move(signature));
	}
}

void ReceiveSlateBuilder::UpdateDatabase(
	Writer<IWalletDB> pBatch,
	const SecureVector& masterSeed,
	const Slate& slate,
	const OutputDataEntity& outputData,
	const uint32_t walletTxId,
	const std::optional<std::string>& addressOpt,
	const std::optional<std::string>& messageOpt) const
{
	// Save secretKey and secretNonce
	//pBatch->SaveSlateContext(masterSeed, slate.GetSlateId(), context);

	// Save OutputDataEntity
	pBatch->AddOutputs(masterSeed, std::vector<OutputDataEntity>({ outputData }));

	// Save WalletTx
	WalletTx walletTx(
		walletTxId,
		EWalletTxType::RECEIVING_IN_PROGRESS,
		std::make_optional(slate.GetSlateId()),
		std::optional<std::string>(addressOpt),
		std::optional<std::string>(messageOpt),
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		slate.GetAmount(),
		0,
		std::optional<uint64_t>(slate.GetFee()),
		std::nullopt,
		slate.GetPaymentProof()
	);

	pBatch->AddTransaction(masterSeed, walletTx);
}