#include "SendSlateBuilder.h"
#include "TransactionBuilder.h"
#include "OutputBuilder.h"
#include "CoinSelection.h"

#include <Wallet/WalletUtil.h>
#include <Crypto/CryptoUtil.h>
#include <Common/Util/FunctionalUtil.h>
#include <Infrastructure/Logger.h>
#include <Consensus/HardForks.h>

static const uint64_t SLATE_VERSION = 2;

SendSlateBuilder::SendSlateBuilder(const Config& config, const INodeClient& nodeClient)
	: m_config(config), m_nodeClient(nodeClient)
{

}

std::unique_ptr<Slate> SendSlateBuilder::BuildSendSlate(
	Wallet& wallet, 
	const SecureVector& masterSeed, 
	const uint64_t amount, 
	const uint64_t feeBase,
	const uint8_t numOutputs,
	const std::optional<std::string>& messageOpt, 
	const ESelectionStrategy& strategy) const
{
	// Set lock_height for transaction kernel (current chain height).
	const uint64_t blockHeight = m_nodeClient.GetChainHeight() + 1;
	const uint16_t headerVersion = Consensus::GetHeaderVersion(m_config.GetEnvironment().GetEnvironmentType(), blockHeight);

	// Select inputs using desired selection strategy.
	const uint8_t totalNumOutputs = numOutputs + 1;
	const uint64_t numKernels = 1;
	const std::vector<OutputData> availableCoins = wallet.GetAllAvailableCoins(masterSeed);
	std::vector<OutputData> inputs = CoinSelection().SelectCoinsToSpend(availableCoins, amount, feeBase, strategy, totalNumOutputs, numKernels);

	// Calculate the fee
	const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), totalNumOutputs, numKernels);

	// Create change outputs with total blinding factor xC
	uint64_t inputTotal = 0;
	for (const OutputData& input : inputs)
	{
		inputTotal += input.GetAmount();
	}

	const uint32_t walletTxId = wallet.GetNextWalletTxId();
	const uint64_t changeAmount = inputTotal - (amount + fee);
	const EBulletproofType bulletproofType = EBulletproofType::ENHANCED;
	const std::vector<OutputData> changeOutputs = OutputBuilder::CreateOutputs(wallet, masterSeed, changeAmount, walletTxId, numOutputs, bulletproofType);

	// Select random transaction offset, and calculate secret key used in kernel signature.
	BlindingFactor transactionOffset = RandomNumberGenerator::GenerateRandom32();
	SecretKey secretKey = CalculatePrivateKey(transactionOffset, inputs, changeOutputs);

	// Select a random nonce kS
	SecretKey secretNonce = *Crypto::GenerateSecureNonce();

	// Build Transaction
	Transaction transaction = TransactionBuilder::BuildTransaction(inputs, changeOutputs, transactionOffset, fee, 0);

	// Add values to Slate for passing to other participants: UUID, inputs, change_outputs, fee, amount, lock_height, kSG, xSG, oS
	SlateVersionInfo slateVersionInfo(SLATE_VERSION, SLATE_VERSION, headerVersion);
	Slate slate(std::move(slateVersionInfo), 2, uuids::uuid_system_generator()(), std::move(transaction), amount, fee, blockHeight, 0);
	AddSenderInfo(slate, secretKey, secretNonce, messageOpt);

	const WalletTx walletTx = BuildWalletTx(wallet, walletTxId, inputs, changeOutputs, slate, messageOpt);

	const SlateContext slateContext(std::move(secretKey), std::move(secretNonce));
	if (!UpdateDatabase(wallet, masterSeed, slate.GetSlateId(), slateContext, changeOutputs, inputs, walletTx))
	{
		LoggerAPI::LogError("SendSlateBuilder::BuildSendSlate - Failed to update database for slate " + uuids::to_string(slate.GetSlateId()));
		return std::unique_ptr<Slate>(nullptr);
	}

	return std::make_unique<Slate>(std::move(slate));
}

// TODO: Memory is not securely erased from these BlindingFactors. A better way of handling this is needed.
SecretKey SendSlateBuilder::CalculatePrivateKey(const BlindingFactor& transactionOffset, const std::vector<OutputData>& inputs, const std::vector<OutputData>& changeOutputs) const
{
	auto getBlindingFactors = [](OutputData& output) -> BlindingFactor { return BlindingFactor(output.GetBlindingFactor().GetBytes()); };

	// Calculate sum inputs blinding factors xI.
	std::vector<BlindingFactor> inputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(inputs, getBlindingFactors);
	std::unique_ptr<BlindingFactor> pInputBFSum = Crypto::AddBlindingFactors(inputBlindingFactors, std::vector<BlindingFactor>());

	// Calculate sum change outputs blinding factors xC.
	std::vector<BlindingFactor> outputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(changeOutputs, getBlindingFactors);
	std::unique_ptr<BlindingFactor> pOutputBFSum = Crypto::AddBlindingFactors(outputBlindingFactors, std::vector<BlindingFactor>());

	// Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
	BlindingFactor totalBlindingExcessSum = CryptoUtil::AddBlindingFactors(pOutputBFSum.get(), pInputBFSum.get());

	// Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
	BlindingFactor privateKeyBF = CryptoUtil::AddBlindingFactors(&totalBlindingExcessSum, &transactionOffset);

	return SecretKey(CBigInteger<32>(privateKeyBF.GetBytes()));
}

void SendSlateBuilder::AddSenderInfo(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce, const std::optional<std::string>& messageOpt) const
{
	std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(secretKey);
	std::unique_ptr<PublicKey> pPublicNonce = Crypto::CalculatePublicKey(secretNonce);
	if (pPublicKey == nullptr || pPublicNonce == nullptr)
	{
		throw CryptoException();
	}

	ParticipantData participantData(0, *pPublicKey, *pPublicNonce);

	// Add message signature
	if (messageOpt.has_value())
	{
		const Hash message = Crypto::Blake2b(std::vector<unsigned char>(messageOpt.value().cbegin(), messageOpt.value().cend()));
		std::unique_ptr<Signature> pMessageSignature = Crypto::SignMessage(secretKey, *pPublicKey, message);
		if (pMessageSignature == nullptr)
		{
			LoggerAPI::LogError("SendSlateBuilder::AddSenderInfo - Failed to sign message for slate " + uuids::to_string(slate.GetSlateId()));
			throw CryptoException();
		}

		participantData.AddMessage(messageOpt.value(), *pMessageSignature);
	}

	slate.AddParticpantData(participantData);
}

WalletTx SendSlateBuilder::BuildWalletTx(Wallet& wallet, const uint32_t walletTxId, const std::vector<OutputData>& inputs, const std::vector<OutputData>& outputs, const Slate& slate, const std::optional<std::string>& messageOpt) const
{
	uint64_t amountDebited = 0;
	for (const OutputData& input : inputs)
	{
		amountDebited += input.GetAmount();
	}

	uint64_t amountCredited = 0;
	for (const OutputData& output : outputs)
	{
		amountCredited += output.GetAmount();
	}

	return WalletTx(
		walletTxId,
		EWalletTxType::SENDING_STARTED, // TODO: Change EWalletTxType to EWalletTxStatus
		std::make_optional<uuids::uuid>(uuids::uuid(slate.GetSlateId())),
		std::optional<std::string>(messageOpt),
		std::chrono::system_clock::now(),
		std::nullopt,
		std::nullopt,
		amountCredited,
		amountDebited,
		std::make_optional<uint64_t>(slate.GetFee()),
		std::make_optional<Transaction>(Transaction(slate.GetTransaction()))
	);
}

// TODO: Use a DB Batch
bool SendSlateBuilder::UpdateDatabase(Wallet& wallet, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& context, const std::vector<OutputData>& changeOutputs, std::vector<OutputData>& coinsToLock, const WalletTx& walletTx) const
{
	// Save secretKey and secretNonce
	if (!wallet.SaveSlateContext(slateId, masterSeed, context))
	{
		LoggerAPI::LogError("SendSlateBuilder::UpdateDatabase - Failed to save context for slate " + uuids::to_string(slateId));
		return false;
	}

	// Save new blinded outputs
	if (!wallet.SaveOutputs(masterSeed, changeOutputs))
	{
		LoggerAPI::LogError("SendSlateBuilder::UpdateDatabase - Failed to save new outputs.");
		return false;
	}

	// Lock coins
	if (!wallet.LockCoins(masterSeed, coinsToLock))
	{
		LoggerAPI::LogError("SendSlateBuilder::UpdateDatabase - Failed to lock coins.");
		return false;
	}

	// Save the log/WalletTx
	if (!wallet.AddWalletTxs(masterSeed, std::vector<WalletTx>({ walletTx })))
	{
		LoggerAPI::LogError("SendSlateBuilder::UpdateDatabase - Failed to create WalletTx");
		return false;
	}

	return true;
}