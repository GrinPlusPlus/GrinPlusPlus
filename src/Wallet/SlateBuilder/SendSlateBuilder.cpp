#include "SendSlateBuilder.h"
#include "TransactionBuilder.h"
#include "OutputBuilder.h"
#include "CoinSelection.h"
#include "SlateUtil.h"

#include <Consensus.h>
#include <Core/Exceptions/WalletException.h>
#include <Core/Util/FeeUtil.h>
#include <Common/Util/FunctionalUtil.h>
#include <Common/Logger.h>
#include <Net/Tor/TorAddressParser.h>
#include <Crypto/ED25519.h>
#include <Wallet/Models/Slatepack/Armor.h>

SendSlateBuilder::SendSlateBuilder(INodeClientConstPtr pNodeClient)
	: m_pNodeClient(pNodeClient)
{

}

Slate SendSlateBuilder::BuildSendSlate(
	Locked<WalletImpl> wallet, 
	const SecureVector& masterSeed, 
	const uint64_t amount, 
	const uint64_t feeBase,
	const uint8_t maxChangeOutputs,
	const bool sendEntireBalance,
	const std::optional<std::string>& addressOpt,
	const SelectionStrategyDTO& strategy,
	const uint16_t slateVersion,
	const std::vector<SlatepackAddress>& recipients) const
{
	const uint8_t numChangeOutputs = sendEntireBalance ? 0 : maxChangeOutputs;
	const uint8_t totalNumOutputs = 1 + numChangeOutputs;
	const uint64_t numKernels = 1;

	// Select inputs using desired selection strategy.
	auto pWallet = wallet.Write();
	const std::vector<OutputDataEntity> availableCoins = pWallet->GetAllAvailableCoins(masterSeed);

	// Filter all coins to find inputs to spend
	std::vector<OutputDataEntity> inputs = availableCoins;
	if (!sendEntireBalance)
	{
		inputs = CoinSelection::SelectCoinsToSpend(
			availableCoins,
			amount,
			feeBase,
			strategy.GetStrategy(),
			strategy.GetInputs(),
			totalNumOutputs,
			numKernels
		);
	}

	const uint64_t inputTotal = std::accumulate(
		inputs.cbegin(), inputs.cend(), (uint64_t)0,
		[](const uint64_t sum, const OutputDataEntity& input) { return sum + input.GetAmount(); }
	);

	// Calculate the fee
	uint64_t fee = FeeUtil::CalculateFee(
		feeBase,
		(int64_t)inputs.size(),
		(int64_t)totalNumOutputs,
		(int64_t)numKernels
	);
	uint64_t amountToSend = sendEntireBalance ? (inputTotal - fee) : amount;
	if (numChangeOutputs == 0)
	{
		assert((inputTotal - amountToSend) >= fee); // TODO: Exception
		fee = inputTotal - amountToSend;
	}

	return Build(
		pWallet.GetShared(),
		masterSeed,
		amountToSend,
		fee,
		sendEntireBalance ? 0 : numChangeOutputs,
		inputs,
		addressOpt,
		slateVersion,
		recipients
	);
}

Slate SendSlateBuilder::Build(
	const std::shared_ptr<WalletImpl>& pWallet,
	const SecureVector& masterSeed,
	const uint64_t amountToSend,
	const uint64_t fee,
	const uint8_t numChangeOutputs,
	std::vector<OutputDataEntity>& inputs,
	const std::optional<std::string>& addressOpt,
	const uint16_t slateVersion,
	const std::vector<SlatepackAddress>& recipients) const
{
	const uint64_t blockHeight = m_pNodeClient->GetChainHeight() + 1;

	auto pBatch = pWallet->GetDatabase().BatchWrite();
	const uint32_t walletTxId = pBatch->GetNextTransactionId();

	// Create change outputs with total blinding factor xC
	std::vector<OutputDataEntity> changeOutputs;
	if (numChangeOutputs > 0)
	{
		const uint64_t inputTotal = std::accumulate(
			inputs.cbegin(), inputs.cend(), (uint64_t)0,
			[](const uint64_t sum, const OutputDataEntity& input) { return sum + input.GetAmount(); }
		);

		const uint64_t changeAmount = inputTotal - (amountToSend + fee);
		changeOutputs = OutputBuilder::CreateOutputs(
			pWallet,
			pBatch.GetShared(),
			masterSeed,
			changeAmount,
			walletTxId,
			numChangeOutputs,
			EBulletproofType::ENHANCED
		);
	}

	// Select random transaction offset, and calculate secret key used in kernel signature.
	BlindingFactor transactionOffset = CSPRNG::GenerateRandom32();
	SigningKeys signing_keys = SlateUtil::CalculateSigningKeys(
		inputs,
		changeOutputs,
		transactionOffset
	);

	// Payment proof
	// KeyChainPath torPath = KeyChainPath::FromString("m/0/1/1");// TODO: pBatch->GetNextChildPath(KeyChainPath::FromString("m/0/1"));
	KeyChainPath torPath = KeyChainPath::FromString("m/0/1").GetChild(pBatch->GetAddressIndex(KeyChainPath::FromString("m/0/0")));

	std::optional<SlatePaymentProof> proofOpt = std::nullopt;
	auto torAddressOpt = addressOpt.has_value() ? TorAddressParser::Parse(addressOpt.value()) : std::nullopt;
	if (torAddressOpt.has_value())
	{
		ed25519_keypair_t torKey = KeyChain::FromSeed(masterSeed).DeriveED25519Key(torPath);
		proofOpt = std::make_optional(SlatePaymentProof::Create(torKey.public_key, torAddressOpt.value().GetPublicKey()));
	}

	// Add values to Slate for passing to other participants: UUID, inputs, change_outputs, fee, amount, lock_height, kSG, xSG, oS

	Slate slate;
	slate.version = slateVersion;
	slate.blockVersion = Consensus::GetHeaderVersion(blockHeight);
	slate.amount = amountToSend;
	slate.fee = Fee::From(fee);
	slate.proofOpt = proofOpt;
	slate.kernelFeatures = EKernelFeatures::DEFAULT_KERNEL;
	slate.sigs.push_back(SlateSignature{
		signing_keys.public_key,
		signing_keys.public_nonce,
		std::nullopt
	});
	slate.offset = transactionOffset;

	std::for_each(
		inputs.cbegin(), inputs.cend(),
		[&slate](const OutputDataEntity& input) {
			slate.AddInput(input.GetFeatures(), input.GetCommitment());
		}
	);

	std::for_each(
		changeOutputs.cbegin(), changeOutputs.cend(),
		[&slate](const OutputDataEntity& output) {
			slate.AddOutput(output.GetFeatures(), output.GetCommitment(), output.GetRangeProof());
		}
	);

	WalletTx walletTx = BuildWalletTx(
		walletTxId,
		transactionOffset,
		inputs,
		changeOutputs,
		slate,
		addressOpt,
		proofOpt
	);

	UpdateDatabase(
		pBatch.GetShared(),
		masterSeed,
		slate,
		SlateContextEntity{ signing_keys.secret_key, signing_keys.secret_nonce, inputs, changeOutputs },
		changeOutputs,
		inputs,
		walletTx,
		Armor::Pack(pWallet->GetSlatepackAddress(), slate, recipients)
	);

	pBatch->Commit();

	return slate;
}

WalletTx SendSlateBuilder::BuildWalletTx(
	const uint32_t walletTxId,
	const BlindingFactor& txOffset,
	const std::vector<OutputDataEntity>& inputs,
	const std::vector<OutputDataEntity>& outputs,
	const Slate& slate,
	const std::optional<std::string>& addressOpt,
	const std::optional<SlatePaymentProof>& proofOpt) const
{
	uint64_t amountDebited = 0;
	for (const OutputDataEntity& input : inputs)
	{
		amountDebited += input.GetAmount();
	}

	uint64_t amountCredited = 0;
	for (const OutputDataEntity& output : outputs)
	{
		amountCredited += output.GetAmount();
	}

	std::vector<TransactionInput> txInputs;
	std::transform(
		inputs.cbegin(), inputs.cend(),
		std::back_inserter(txInputs),
		[](const OutputDataEntity& input) { return TransactionInput{ input.GetFeatures(), input.GetCommitment() }; }
	);

	std::vector<TransactionOutput> txOutputs;
	std::transform(
		outputs.cbegin(), outputs.cend(),
		std::back_inserter(txOutputs),
		[](const OutputDataEntity& output) { return TransactionOutput{ output.GetFeatures(), output.GetCommitment(), output.GetRangeProof() }; }
	);

	TransactionKernel kernel{ slate.kernelFeatures, slate.fee, slate.GetLockHeight(), Commitment(), Signature() };
	Transaction transaction = TransactionBuilder::BuildTransaction(txInputs, txOutputs, std::move(kernel), txOffset);

	return WalletTx(
		walletTxId,
		EWalletTxType::SENDING_STARTED, // TODO: Change EWalletTxType to EWalletTxStatus
		std::make_optional(slate.GetId()),
		std::optional<std::string>(addressOpt),
		std::nullopt,
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		amountCredited,
		amountDebited,
		std::make_optional(slate.GetFee()),
		std::make_optional(std::move(transaction)),
		proofOpt.has_value() ? std::make_optional(proofOpt.value()) : std::nullopt
	);
}

void SendSlateBuilder::UpdateDatabase(
	std::shared_ptr<IWalletDB> pBatch,
	const SecureVector& masterSeed,
	const Slate& slate,
	const SlateContextEntity& context,
	const std::vector<OutputDataEntity>& changeOutputs,
	std::vector<OutputDataEntity>& coinsToLock,
	const WalletTx& walletTx,
	const std::string& armored_slatepack) const
{
	pBatch->SaveSlateContext(masterSeed, slate.GetId(), context);
	pBatch->SaveSlate(masterSeed, slate, armored_slatepack);
	pBatch->AddOutputs(masterSeed, changeOutputs);

	// Lock coins
	for (OutputDataEntity& coin : coinsToLock)
	{
		coin.SetStatus(EOutputStatus::LOCKED);
	}

	pBatch->AddOutputs(masterSeed, coinsToLock);
	pBatch->AddTransaction(masterSeed, walletTx);
}