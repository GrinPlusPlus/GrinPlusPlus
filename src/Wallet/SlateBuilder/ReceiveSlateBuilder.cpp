#include "ReceiveSlateBuilder.h"
#include "SignatureUtil.h"
#include "TransactionBuilder.h"
#include "SlateUtil.h"

#include <Core/Exceptions/WalletException.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Common/Logger.h>

Slate ReceiveSlateBuilder::AddReceiverData(
	Locked<WalletImpl> wallet,
	const SecureVector& masterSeed,
	const Slate& send_slate,
	const std::optional<SlatepackAddress>& addressOpt,
	const std::vector<SlatepackAddress>& recipients) const
{
	WALLET_INFO_F(
		"Receiving {} from {}",
		send_slate.GetAmount(),
		addressOpt.has_value() ? addressOpt.value().ToString() : "UNKNOWN"
	);

	auto pWallet = wallet.Write();
	Slate receiveSlate = send_slate;
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

	const BlindingFactor receiver_offset = CSPRNG::GenerateRandom32();
	SigningKeys signing_keys = SlateUtil::CalculateSigningKeys({}, { outputData }, receiver_offset);

	// Adjust transaction offset
	receiveSlate.offset = Crypto::AddBlindingFactors({ receiveSlate.offset, receiver_offset }, {});
	WALLET_INFO_F(
		"Adjusting original transaction offset {} by {} to get total {}",
		send_slate.offset,
		receiver_offset,
		receiveSlate.offset
	);

	// Add receiver ParticipantData
	SlateSignature signature = BuildSignature(receiveSlate, signing_keys);

	// Add output to Transaction
	receiveSlate.AddOutput(outputData.GetFeatures(), outputData.GetCommitment(), outputData.GetRangeProof());

	UpdatePaymentProof(
		pWallet.GetShared(),
		masterSeed,
		receiveSlate
	);
	
	receiveSlate.sigs = std::vector<SlateSignature>{ signature };
	UpdateDatabase(
		pBatch.GetShared(),
		masterSeed,
		receiveSlate,
		outputData,
		walletTxId,
		addressOpt,
		Armor::Pack(pWallet->GetSlatepackAddress(), receiveSlate, recipients)
	);

	pBatch->Commit();

	return receiveSlate;
}

bool ReceiveSlateBuilder::VerifySlateStatus(
	std::shared_ptr<WalletImpl> pWallet,
	const SecureVector& masterSeed,
	const Slate& slate) const
{
	// Slate was already received.
	std::unique_ptr<WalletTx> pWalletTx = pWallet->GetTxBySlateId(masterSeed, slate.GetId());
	if (pWalletTx != nullptr) {
		const EWalletTxType status = pWalletTx->GetType();
		if (status != EWalletTxType::RECEIVED_CANCELED && status != EWalletTxType::SENDING_STARTED) {
			WALLET_ERROR_F("Already received slate {}", uuids::to_string(slate.GetId()));
			return false;
		}
	}

	return true;
}

SlateSignature ReceiveSlateBuilder::BuildSignature(Slate& slate, const SigningKeys& signing_keys) const
{
	const Hash kernelMessage = TransactionKernel::GetSignatureMessage(
		slate.kernelFeatures,
		slate.fee,
		slate.GetLockHeight()
	);

	// Generate signature
	slate.sigs.push_back(SlateSignature{ signing_keys.public_key, signing_keys.public_nonce, std::nullopt });

	CompactSignature partialSignature = Crypto::CalculatePartialSignature(
		signing_keys.secret_key,
		signing_keys.secret_nonce,
		slate.CalculateTotalExcess(),
		slate.CalculateTotalNonce(),
		kernelMessage
	);

	slate.sigs.back().partialOpt = std::make_optional<CompactSignature>(partialSignature);

	if (!SignatureUtil::VerifyPartialSignatures(slate.sigs, kernelMessage)) {
		WALLET_ERROR_F("Failed to verify signature for slate {}", uuids::to_string(slate.GetId()));
		throw WALLET_EXCEPTION("Failed to verify signatures.");
	}

	// Add receiver's ParticipantData to Slate
	return slate.sigs.back();
}

void ReceiveSlateBuilder::UpdatePaymentProof(
	const std::shared_ptr<WalletImpl>& pWallet,
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
			throw WALLET_EXCEPTION_F("Payment proof receiver address ({} - {}) does not match wallet's (TOR: [{} - {}], Slatepack: [{} - {}])",
				proof.GetReceiverAddress().Format(),
				SlatepackAddress(proof.GetReceiverAddress()).ToString(),
				pWallet->GetTorAddress().value().GetPublicKey().Format(),
				SlatepackAddress(pWallet->GetTorAddress().value().GetPublicKey()).ToString(),
				pWallet->GetSlatepackAddress().GetEdwardsPubKey().Format(),
				pWallet->GetSlatepackAddress().ToString()
			);
		}

		Commitment kernel_commitment = Crypto::ToCommitment(receiveSlate.CalculateTotalExcess());
		WALLET_INFO_F("Calculated commitment: {}", kernel_commitment);

		// Message: (amount | kernel commitment | sender address)
		Serializer messageSerializer;
		messageSerializer.Append<uint64_t>(receiveSlate.GetAmount());
		kernel_commitment.Serialize(messageSerializer);
		messageSerializer.AppendBigInteger(proof.GetSenderAddress().bytes);

		KeyChainPath userPath = KeyChainPath::FromString("m/0/0");
		int currentAddressIndex = pWallet->GetDatabase().Read()->GetAddressIndex(userPath);

		KeyChain keyChain = KeyChain::FromSeed(masterSeed);
		ed25519_keypair_t torKey = keyChain.DeriveED25519Key(KeyChainPath::FromString("m/0/1").GetChild(currentAddressIndex));

		ed25519_signature_t signature = ED25519::Sign(
			torKey.secret_key,
			messageSerializer.GetBytes()
		);
		proof.AddSignature(std::move(signature));
	}
}

void ReceiveSlateBuilder::UpdateDatabase(
	std::shared_ptr<IWalletDB> pBatch,
	const SecureVector& masterSeed,
	const Slate& receiveSlate,
	const OutputDataEntity& outputData,
	const uint32_t walletTxId,
	const std::optional<SlatepackAddress>& addressOpt,
	const std::string& armored_slatepack) const
{
	// Save OutputDataEntity
	pBatch->AddOutputs(masterSeed, std::vector<OutputDataEntity>{ outputData });

	// Save WalletTx
	WalletTx walletTx(
		walletTxId,
		EWalletTxType::RECEIVING_IN_PROGRESS,
		std::make_optional(receiveSlate.GetId()),
		addressOpt.has_value() ? std::make_optional(addressOpt.value().ToString()) : std::nullopt,
		std::nullopt,
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		receiveSlate.amount,
		0,
		std::make_optional<Fee>(receiveSlate.fee),
		std::nullopt,
		receiveSlate.GetPaymentProof()
	);

	pBatch->AddTransaction(masterSeed, walletTx);

	// receiveSlate.amount = 0;
	// receiveSlate.fee = 0;
	pBatch->SaveSlate(masterSeed, receiveSlate, armored_slatepack);
}