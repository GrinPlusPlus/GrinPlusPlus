#include "SendSlateBuilder.h"
#include "TransactionBuilder.h"
#include "OutputBuilder.h"
#include "CoinSelection.h"

#include <Core/Exceptions/WalletException.h>
#include <Wallet/WalletUtil.h>
#include <Crypto/CryptoUtil.h>
#include <Common/Util/FunctionalUtil.h>
#include <Infrastructure/Logger.h>
#include <Consensus/HardForks.h>
#include <Net/Tor/TorAddressParser.h>
#include <Crypto/ED25519.h>

SendSlateBuilder::SendSlateBuilder(const Config& config, INodeClientConstPtr pNodeClient)
	: m_config(config), m_pNodeClient(pNodeClient)
{

}

Slate SendSlateBuilder::BuildSendSlate(
	Locked<Wallet> wallet, 
	const SecureVector& masterSeed, 
	const uint64_t amount, 
	const uint64_t feeBase,
	const uint8_t maxChangeOutputs,
	const bool sendEntireBalance,
	const std::optional<std::string>& addressOpt,
	const SelectionStrategyDTO& strategy,
	const uint16_t slateVersion) const
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
	uint64_t fee = WalletUtil::CalculateFee(
		feeBase,
		inputs.size(),
		totalNumOutputs,
		numKernels
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
		slateVersion
	);
}

Slate SendSlateBuilder::Build(
	const std::shared_ptr<Wallet>& pWallet,
	const SecureVector& masterSeed,
	const uint64_t amountToSend,
	const uint64_t fee,
	const uint8_t numChangeOutputs,
	std::vector<OutputDataEntity>& inputs,
	const std::optional<std::string>& addressOpt,
	const uint16_t slateVersion) const
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
	BlindingFactor transactionOffset = RandomNumberGenerator::GenerateRandom32();
	SecretKey secretKey = CalculatePrivateKey(transactionOffset, inputs, changeOutputs);

	// Select a random nonce kS
	SecretKey secretNonce = Crypto::GenerateSecureNonce();

	// Build Transaction
	//Transaction transaction = TransactionBuilder::BuildTransaction(inputs, changeOutputs, transactionOffset, fee, 0);

	// Payment proof
	KeyChainPath torPath = KeyChainPath::FromString("m/0/1/1");// TODO: pBatch->GetNextChildPath(KeyChainPath::FromString("m/0/1"));

	std::optional<SlatePaymentProof> proofOpt = std::nullopt;
	auto torAddressOpt = addressOpt.has_value() ? TorAddressParser::Parse(addressOpt.value()) : std::nullopt;
	if (torAddressOpt.has_value())
	{
		ed25519_keypair_t torKey = KeyChain::FromSeed(m_config, masterSeed).DeriveED25519Key(torPath);
		proofOpt = std::make_optional(SlatePaymentProof::Create(torKey.public_key, torAddressOpt.value().GetPublicKey()));
	}

	// Add values to Slate for passing to other participants: UUID, inputs, change_outputs, fee, amount, lock_height, kSG, xSG, oS

	Slate slate;
	slate.version = slateVersion;
	slate.blockVersion = Consensus::GetHeaderVersion(m_config.GetEnvironment().GetType(), blockHeight);
	slate.amount = amountToSend;
	slate.fee = fee;
	slate.proofOpt = proofOpt;
	slate.kernelFeatures = EKernelFeatures::DEFAULT_KERNEL;
	slate.sigs.push_back(SlateSignature{
		Crypto::CalculatePublicKey(secretKey),
		Crypto::CalculatePublicKey(secretNonce),
		std::nullopt
	});

	std::for_each(
		inputs.cbegin(), inputs.cend(),
		[&slate](const OutputDataEntity& input) {
			slate.commitments.push_back(SlateCommitment{ input.GetFeatures(), input.GetCommitment(), std::nullopt });
		}
	);

	std::for_each(
		changeOutputs.cbegin(), changeOutputs.cend(),
		[&slate](const OutputDataEntity& output) {
			slate.commitments.push_back(SlateCommitment{ output.GetFeatures(), output.GetCommitment(), output.GetRangeProof() });
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
		SlateContextEntity{ secretKey, secretNonce, inputs, changeOutputs },
		changeOutputs,
		inputs,
		walletTx
	);

	pBatch->Commit();

	return slate;
}

SecretKey SendSlateBuilder::CalculatePrivateKey(
	const BlindingFactor& transactionOffset,
	const std::vector<OutputDataEntity>& inputs,
	const std::vector<OutputDataEntity>& changeOutputs) const
{
	// Calculate sum inputs blinding factors xI.
	std::vector<BlindingFactor> inputBlindingFactors;
	std::transform(
		inputs.begin(), inputs.end(),
		std::back_inserter(inputBlindingFactors), 
		[](const OutputDataEntity& output) { return BlindingFactor(output.GetBlindingFactor().GetBytes()); }
	);
	BlindingFactor inputBFSum = Crypto::AddBlindingFactors(inputBlindingFactors, {});

	// Calculate sum change outputs blinding factors xC.
	std::vector<BlindingFactor> outputBlindingFactors;
	std::transform(
		changeOutputs.begin(), changeOutputs.end(),
		std::back_inserter(outputBlindingFactors),
		[](const OutputDataEntity& output) { return BlindingFactor(output.GetBlindingFactor().GetBytes()); }
	);
	BlindingFactor outputBFSum = Crypto::AddBlindingFactors(outputBlindingFactors, {});

	// Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
	BlindingFactor totalBlindingExcessSum = CryptoUtil::AddBlindingFactors(&outputBFSum, &inputBFSum);

	// Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
	BlindingFactor privateKeyBF = CryptoUtil::AddBlindingFactors(&totalBlindingExcessSum, &transactionOffset);

	return privateKeyBF.ToSecretKey();
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
	const WalletTx& walletTx) const
{
	// Save private context
	pBatch->SaveSlateContext(masterSeed, slate.GetId(), context);

	// Save slate
	pBatch->SaveSlate(masterSeed, slate);

	// Save new blinded outputs
	pBatch->AddOutputs(masterSeed, changeOutputs);

	// Lock coins
	for (OutputDataEntity& coin : coinsToLock)
	{
		coin.SetStatus(EOutputStatus::LOCKED);
	}

	pBatch->AddOutputs(masterSeed, coinsToLock);

	// Save the log/WalletTx
	pBatch->AddTransaction(masterSeed, walletTx);
}