#include "SlateBuilder.h"
#include "WalletUtil.h"
#include "WalletCoin.h"

#include <Wallet/InsufficientFundsException.h>
#include <Wallet/SlateContext.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Crypto/CryptoUtil.h>
#include <Common/Util/FunctionalUtil.h>
#include <Infrastructure/Logger.h>

static const uint64_t SLATE_VERSION = 1;

SlateBuilder::SlateBuilder(const INodeClient& nodeClient)
	: m_nodeClient(nodeClient)
{

}

/*
	1: Create Transaction **UUID** (for reference and maintaining correct state)
	2: Set **lock_height** for transaction kernel (current chain height)
	3: Select **inputs** using desired selection strategy
	4: Calculate sum **inputs** blinding factors **xI**
	5: Create **change_output**
	6: Select blinding factor **xC** for **change_output**
	7: Create lock function **sF** that locks **inputs** and stores **change_output** in wallet
	   and identifying wallet transaction log entry **TS** linking **inputs + outputs**
	   (Not executed at this point)
	8: Calculate **tx_weight**: MAX(-1 * **num_inputs** + 4 * (**num_change_outputs** + 1), 1)
		(+1 covers a single output on the receiver's side)
	9: Calculate **fee**:  **tx_weight** * 1_000_000 nG
	10: Calculate total blinding excess sum for all inputs and outputs **xS1** = **xC** - **xI** (private scalar)
	11: Select a random nonce **kS** (private scalar)
	12: Subtract random kernel offset **oS** from **xS1**. Calculate **xS** = **xS1** - **oS**
	13: Multiply **xS** and **kS** by generator G to create public curve points **xSG** and **kSG**
	14: Add values to **Slate** for passing to other participants: **UUID, inputs, change_outputs,**
		**fee, amount, lock_height, kSG, xSG, oS**
*/
std::unique_ptr<Slate> SlateBuilder::BuildSendSlate(Wallet& wallet, const CBigInteger<32>& masterSeed, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy) const
{
	// Create Transaction UUID (for reference and maintaining correct state).
	uuids::uuid slateId = uuids::uuid_system_generator()();

	// Set lock_height for transaction kernel (current chain height).
	const uint64_t lockHeight = m_nodeClient.GetChainHeight() + 1;

	// Select inputs using desired selection strategy.
	const uint64_t numOutputs = 2;
	const uint64_t numKernels = 1;
	std::vector<WalletCoin> inputs = SelectCoinsToSpend(wallet, masterSeed, amount, feeBase, strategy, numOutputs, numKernels);
	
	// Calculate sum inputs blinding factors xI.
	auto getInputBlindingFactors = [wallet](WalletCoin& input) -> BlindingFactor { return input.GetBlindingFactor(); };
	std::vector<BlindingFactor> inputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(inputs, getInputBlindingFactors);
	std::unique_ptr<BlindingFactor> pBlindingFactorSum = Crypto::AddBlindingFactors(inputBlindingFactors, std::vector<BlindingFactor>());
	
	// Calculate the fee
	const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), 2, 1);

	// Create change output with blinding factor xC
	std::unique_ptr<WalletCoin> pChangeOutput = CreateChangeOutput(wallet, masterSeed, inputs, amount, fee);

	// Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
	BlindingFactor totalBlindingExcessSum = CryptoUtil::AddBlindingFactors(&pChangeOutput->GetBlindingFactor(), pBlindingFactorSum.get());

	// Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
	BlindingFactor transactionOffset = RandomNumberGenerator::GenerateRandom32();
	BlindingFactor secretKey = CryptoUtil::AddBlindingFactors(&totalBlindingExcessSum, &transactionOffset);
	std::unique_ptr<CBigInteger<33>> pPublicKey = Crypto::SECP256K1_CalculateCompressedPublicKey(secretKey);

	// Select a random nonce kS
	BlindingFactor secretNonce = RandomNumberGenerator::GenerateRandom32();
	std::unique_ptr<CBigInteger<33>> pPublicNonce = Crypto::SECP256K1_CalculateCompressedPublicKey(secretNonce);

	// Build Transaction
	Transaction transaction = BuildTransaction(inputs, *pChangeOutput, transactionOffset, fee, lockHeight);

	// Save secretKey and secretNonce
	if (!wallet.SaveSlateContext(slateId, masterSeed, SlateContext(std::move(secretKey), std::move(secretNonce))))
	{
		LoggerAPI::LogError("SlateBuilder::BuildSendSlate - Failed to save context for slate " + uuids::to_string(slateId));
		return std::unique_ptr<Slate>(nullptr);
	}

	// Lock coins
	if (!wallet.LockCoins(masterSeed, inputs))
	{
		LoggerAPI::LogError("SlateBuilder::BuildSendSlate - Failed to lock coins.");
		return std::unique_ptr<Slate>(nullptr);
	}

	// Add values to Slate for passing to other participants: UUID, inputs, change_outputs, fee, amount, lock_height, kSG, xSG, oS
	std::unique_ptr<Slate> pSlate = std::make_unique<Slate>(Slate(SLATE_VERSION, 2, std::move(slateId), std::move(transaction), amount, fee, lockHeight, lockHeight));

	ParticipantData participantData(0, *pPublicKey, *pPublicNonce);
	// TODO: Add message signature
	pSlate->AddParticpantData(participantData);

	return pSlate;
}

bool SlateBuilder::AddReceiverData(Wallet& wallet, const CBigInteger<32>& masterSeed, Slate& slate, const std::optional<std::string>& messageOpt) const
{
	// TODO: Verify fees

	// Generate message
	const Transaction& transaction = slate.GetTransaction();
	const std::vector<TransactionKernel>& kernels = transaction.GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		return false;
	}

	const TransactionKernel& kernel = kernels.front();
	const Hash message = kernel.GetSignatureMessage();

	// Generate output
	std::unique_ptr<WalletCoin> pWalletCoin = wallet.CreateBlindedOutput(masterSeed, slate.GetAmount());
	BlindingFactor secretKey = pWalletCoin->GetBlindingFactor();
	std::unique_ptr<CBigInteger<33>> pPublicBlindExcess = Crypto::SECP256K1_CalculateCompressedPublicKey(secretKey);

	// Generate nonce
	BlindingFactor secretNonce = RandomNumberGenerator::GenerateRandom32();
	std::unique_ptr<CBigInteger<33>> pPublicNonce = Crypto::SECP256K1_CalculateCompressedPublicKey(secretNonce);

	// Build receiver's ParticipantData
	ParticipantData receiverData(1, *pPublicBlindExcess, *pPublicNonce);

	// Generate signature
	std::vector<ParticipantData> participants = slate.GetParticipantData();
	participants.emplace_back(receiverData);

	std::unique_ptr<Signature> pPartialSignature = GeneratePartialSignature(secretKey, secretNonce, participants, message);
	if (pPartialSignature == nullptr)
	{
		LoggerAPI::LogError("SlateBuilder::AddReceiverData - Failed to generate signature for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	receiverData.AddPartialSignature(*pPartialSignature);

	// Save secretKey and secretNonce
	if (!wallet.SaveSlateContext(slate.GetSlateId(), masterSeed, SlateContext(std::move(secretKey), std::move(secretNonce))))
	{
		LoggerAPI::LogError("SlateBuilder::AddReceiverData - Failed to save context for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	// TODO: Add output to Transaction
	// TODO: Add message signature
	// Add receiver's ParticipantData to Slate
	slate.AddParticpantData(receiverData);

	return true;
}

std::unique_ptr<Transaction> SlateBuilder::Finalize(Wallet& wallet, const CBigInteger<32>& masterSeed, const Slate& slate) const
{
	// TODO: Verify partial signatures

	// Load secretKey and secretNonce
	std::unique_ptr<SlateContext> pSlateContext = wallet.GetSlateContext(slate.GetSlateId(), masterSeed);
	if (pSlateContext == nullptr)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const BlindingFactor& secretKey = pSlateContext->GetSecretKey();
	std::unique_ptr<CBigInteger<33>> pPublicKey = Crypto::SECP256K1_CalculateCompressedPublicKey(secretKey);

	const BlindingFactor& secretNonce = pSlateContext->GetSecretNonce();
	std::unique_ptr<CBigInteger<33>> pPublicNonce = Crypto::SECP256K1_CalculateCompressedPublicKey(secretNonce);

	// Generate message
	const Transaction& transaction = slate.GetTransaction();
	const std::vector<TransactionKernel>& kernels = transaction.GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const TransactionKernel& kernel = kernels.front();
	const Hash message = kernel.GetSignatureMessage();

	// Generate partial signature
	std::unique_ptr<Signature> pPartialSignature = GeneratePartialSignature(secretKey, secretNonce, slate.GetParticipantData(), message);

	// TODO: Aggregate partial signatures
	std::unique_ptr<Signature> pAggSignature = std::unique_ptr<Signature>(nullptr);

	// Build the final excess based on final tx and offset
	auto getInputCommitments = [](const TransactionInput& input) -> Commitment { return input.GetCommitment(); };
	std::vector<Commitment> inputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetInputs(), getInputCommitments);
	inputCommitments.emplace_back(*Crypto::CommitBlinded(0, transaction.GetOffset()));

	auto getOutputCommitments = [](const TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
	std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetOutputs(), getOutputCommitments);
	outputCommitments.emplace_back(*Crypto::CommitTransparent(slate.GetFee()));

	std::unique_ptr<Commitment> pFinalExcess = Crypto::AddCommitments(outputCommitments, inputCommitments);

	// Update the tx kernel to reflect the offset excess and sig
	TransactionKernel finalKernel(kernel.GetFeatures(), kernel.GetFee(), kernel.GetLockHeight(), Commitment(*pFinalExcess), Signature(*pAggSignature));
	// TODO: Verify finalKernel && final Transaction

	return std::unique_ptr<Transaction>(nullptr);
}

// TODO: Apply Strategy instead of just selecting greatest number of outputs.
// If strategy is "ALL", spend all available coins to reduce the fee.
std::vector<WalletCoin> SlateBuilder::SelectCoinsToSpend(Wallet& wallet, const CBigInteger<32>& masterSeed, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const
{
	std::vector<WalletCoin> availableCoins = wallet.GetAllAvailableCoins(masterSeed);
	std::sort(availableCoins.begin(), availableCoins.end());

	uint64_t amountFound = 0;
	std::vector<WalletCoin> selectedCoins;
	for (const WalletCoin& coin : availableCoins)
	{
		amountFound += coin.GetOutputData().GetAmount();
		selectedCoins.push_back(coin);

		const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)selectedCoins.size(), numOutputs, numKernels);
		if (amountFound >= (amount + fee))
		{
			return selectedCoins;
		}
	}

	// Not enough coins found.
	LoggerAPI::LogError("SlateBuilder::SelectCoinsToSpend - Not enough funds.");
	throw InsufficientFundsException();
}

std::unique_ptr<WalletCoin> SlateBuilder::CreateChangeOutput(Wallet& wallet, const CBigInteger<32>& masterSeed, const std::vector<WalletCoin>& inputs, const uint64_t amount, const uint64_t fee) const
{
	uint64_t inputTotal = 0;
	for (const WalletCoin& input : inputs)
	{
		inputTotal += input.GetOutputData().GetAmount();
	}
	const uint64_t changeAmount = inputTotal - (amount + fee);

	return wallet.CreateBlindedOutput(masterSeed, changeAmount);
}

Transaction SlateBuilder::BuildTransaction(const std::vector<WalletCoin>& inputs, const WalletCoin& changeOutput, const BlindingFactor& transactionOffset, const uint64_t fee, const uint64_t lockHeight) const
{
	auto getInput = [](WalletCoin& input) -> TransactionInput { return TransactionInput(input.GetOutputData().GetOutput().GetFeatures(), Commitment(input.GetOutputData().GetOutput().GetCommitment())); };
	std::vector<TransactionInput> transactionInputs = FunctionalUtil::map<std::vector<TransactionInput>>(inputs, getInput);

	std::vector<TransactionOutput> transactionOutputs({ changeOutput.GetOutputData().GetOutput() });

	TransactionKernel kernel(EKernelFeatures::HEIGHT_LOCKED, fee, lockHeight, Commitment(CBigInteger<33>::ValueOf(0)), Signature(CBigInteger<64>::ValueOf(0)));
	std::vector<TransactionKernel> kernels({ kernel });

	return Transaction(BlindingFactor(transactionOffset), TransactionBody(std::move(transactionInputs), std::move(transactionOutputs), std::move(kernels)));
}

std::unique_ptr<Signature> SlateBuilder::GeneratePartialSignature(const BlindingFactor& secretKey, const BlindingFactor& secretNonce, const std::vector<ParticipantData>& participants, const Hash& message) const
{
	std::vector<CBigInteger<33>> pubKeys;
	std::vector<CBigInteger<33>> pubNonces;

	for (const ParticipantData& participantData : participants)
	{
		pubKeys.push_back(participantData.GetPublicBlindExcess());
		pubNonces.push_back(participantData.GetPublicNonce());
	}

	const CBigInteger<33> sumPubKeys = CryptoUtil::AddPublicKeys(pubKeys);
	const CBigInteger<33> sumPubNonces = CryptoUtil::AddPublicKeys(pubNonces);

	return Crypto::CalculatePartialSignature(secretKey, secretNonce, sumPubKeys, sumPubNonces, message);
}