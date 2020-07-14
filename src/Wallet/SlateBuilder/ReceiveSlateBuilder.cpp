#include "ReceiveSlateBuilder.h"
#include "SignatureUtil.h"
#include "TransactionBuilder.h"
#include "SlateUtil.h"

#include <Core/Exceptions/WalletException.h>
#include <Wallet/WalletUtil.h>
#include <Infrastructure/Logger.h>

Slate ReceiveSlateBuilder::AddReceiverData(
	Locked<Wallet> wallet,
	const SecureVector& masterSeed,
	const Slate& slate,
	const std::optional<std::string>& addressOpt) const
{
	WALLET_INFO_F("Receiving {} from {}", slate.GetAmount(), addressOpt.value_or("UNKNOWN"));

	auto pWallet = wallet.Write();
	Slate receiveSlate = slate;
	receiveSlate.stage = ESlateStage::STANDARD_RECEIVED;

	// Verify slate was never received, exactly one kernel exists, and fee is valid.
	if (!VerifySlateStatus(pWallet.GetShared(), masterSeed, receiveSlate))
	{
		throw WALLET_EXCEPTION("Failed to verify received slate.");
	}

	auto pBatch = pWallet->GetDatabase().BatchWrite();

	// Generate output
	KeyChainPath keyChainPath = pBatch->GetNextChildPath(pWallet->GetUserPath());
	const uint32_t walletTxId = pBatch->GetNextTransactionId();
	OutputDataEntity outputData = pWallet->CreateBlindedOutput(
		masterSeed,
		receiveSlate.GetAmount(),
		keyChainPath,
		walletTxId,
		EBulletproofType::ENHANCED
	);
	const SecretKey& secretKey = outputData.GetBlindingFactor();
	SecretKey secretNonce = Crypto::GenerateSecureNonce();

	// TODO: Adjust offset?

	// Add receiver ParticipantData
	SlateSignature signature = BuildSignature(receiveSlate, secretKey, secretNonce);

	// Add output to Transaction
	SlateCommitment outputCommit{
		outputData.GetFeatures(),
		outputData.GetCommitment(),
		std::make_optional<RangeProof>(outputData.GetRangeProof())
	};
	receiveSlate.commitments.push_back(outputCommit);

	UpdatePaymentProof(
		pWallet.GetShared(),
		pBatch.GetShared(),
		masterSeed,
		receiveSlate
	);

	UpdateDatabase(
		pBatch.GetShared(),
		masterSeed,
		receiveSlate,
		signature,
		outputData,
		walletTxId,
		addressOpt
	);

	pBatch->Commit();

	return receiveSlate;
}

bool ReceiveSlateBuilder::VerifySlateStatus(
	std::shared_ptr<Wallet> pWallet,
	const SecureVector& masterSeed,
	const Slate& slate) const
{
	// Slate was already received.
	std::unique_ptr<WalletTx> pWalletTx = pWallet->GetTxBySlateId(masterSeed, slate.GetId());
	if (pWalletTx != nullptr && pWalletTx->GetType() != EWalletTxType::RECEIVED_CANCELED)
	{
		WALLET_ERROR_F("Already received slate {}", uuids::to_string(slate.GetId()));
		return false;
	}

	// TODO: Verify fees

	return true;
}

SlateSignature ReceiveSlateBuilder::BuildSignature(
	Slate& slate,
	const SecretKey& secretKey,
	const SecretKey& secretNonce) const
{
	const Hash kernelMessage = TransactionKernel::GetSignatureMessage(
		slate.kernelFeatures,
		slate.fee,
		slate.GetLockHeight()
	);

	PublicKey excess = Crypto::CalculatePublicKey(secretKey);
	PublicKey nonce = Crypto::CalculatePublicKey(secretNonce);

	// Generate signature
	std::vector<SlateSignature> signatures = slate.sigs;
	signatures.push_back(SlateSignature{ excess, nonce, std::nullopt });

	std::unique_ptr<CompactSignature> pPartialSignature = SignatureUtil::GeneratePartialSignature(
		secretKey,
		secretNonce,
		signatures,
		kernelMessage
	);
	if (pPartialSignature == nullptr)
	{
		WALLET_ERROR_F("Failed to generate signature for slate {}", uuids::to_string(slate.GetId()));
		throw WALLET_EXCEPTION("Failed to generate signature.");
	}

	signatures.back().partialOpt = std::make_optional<CompactSignature>(*pPartialSignature);

	if (!SignatureUtil::VerifyPartialSignatures(signatures, kernelMessage))
	{
		WALLET_ERROR_F("Failed to verify signature for slate {}", uuids::to_string(slate.GetId()));
		throw WALLET_EXCEPTION("Failed to verify signatures.");
	}

	// Add receiver's ParticipantData to Slate
	return signatures.back();
}

void ReceiveSlateBuilder::UpdatePaymentProof(
	std::shared_ptr<Wallet> pWallet,
	IWalletDBPtr pWalletDB,
	const SecureVector& masterSeed,
	Slate& receiveSlate) const
{
	if (receiveSlate.GetPaymentProof().has_value())
	{
		if (!pWallet->GetTorAddress().has_value())
		{
			throw WALLET_EXCEPTION("");
		}

		auto& proof = receiveSlate.GetPaymentProof().value();
		if (proof.GetReceiverAddress() != pWallet->GetTorAddress().value().GetPublicKey())
		{
			throw WALLET_EXCEPTION("");
		}

		// Message: (amount | kernel commitment | sender address)
		Serializer messageSerializer;
		messageSerializer.Append<uint64_t>(receiveSlate.GetAmount());
		SlateUtil::CalculateFinalExcess(receiveSlate).Serialize(messageSerializer);
		messageSerializer.AppendBigInteger(CBigInteger<32>(proof.GetSenderAddress().pubkey));

		KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);
		SecretKey64 torKey = keyChain.DeriveED25519Key(KeyChainPath::FromString("m/0/1/0"));

		Signature signature = ED25519::Sign(
			torKey,
			proof.GetReceiverAddress(),
			messageSerializer.GetBytes()
		);
		proof.AddSignature(std::move(signature));
	}
}

void ReceiveSlateBuilder::UpdateDatabase(
	std::shared_ptr<IWalletDB> pBatch,
	const SecureVector& masterSeed,
	Slate& receiveSlate,
	const SlateSignature& signature,
	const OutputDataEntity& outputData,
	const uint32_t walletTxId,
	const std::optional<std::string>& addressOpt) const
{
	// Save OutputDataEntity
	pBatch->AddOutputs(masterSeed, std::vector<OutputDataEntity>{ outputData });

	// Save WalletTx
	WalletTx walletTx(
		walletTxId,
		EWalletTxType::RECEIVING_IN_PROGRESS,
		std::make_optional(receiveSlate.GetId()),
		std::optional<std::string>(addressOpt),
		std::nullopt,
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		receiveSlate.amount,
		0,
		std::make_optional<uint64_t>(receiveSlate.fee),
		std::nullopt,
		receiveSlate.GetPaymentProof()
	);

	pBatch->AddTransaction(masterSeed, walletTx);

	receiveSlate.amount = 0;
	receiveSlate.fee = 0;
	receiveSlate.sigs = std::vector<SlateSignature>{ signature };
	pBatch->SaveSlate(masterSeed, receiveSlate);
}