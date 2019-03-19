#include "SlateBuilder.h"
#include "WalletUtil.h"
#include "TransactionBuilder.h"
#include "SignatureUtil.h"
#include "CoinSelection.h"

#include <Core/Validation/TransactionValidator.h>
#include <Core/Validation/KernelSignatureValidator.h>
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
	const std::vector<OutputData> availableCoins = wallet.GetAllAvailableCoins(masterSeed);
	std::vector<OutputData> inputs = CoinSelection().SelectCoinsToSpend(availableCoins, amount, feeBase, strategy, numOutputs, numKernels);
	
	// Calculate sum inputs blinding factors xI.
	auto getInputBlindingFactors = [wallet](OutputData& input) -> BlindingFactor { return input.GetBlindingFactor(); };
	std::vector<BlindingFactor> inputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(inputs, getInputBlindingFactors);
	std::unique_ptr<BlindingFactor> pBlindingFactorSum = Crypto::AddBlindingFactors(inputBlindingFactors, std::vector<BlindingFactor>());
	
	// Calculate the fee
	const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), numOutputs, numKernels);

	// Create change output with blinding factor xC
	std::unique_ptr<OutputData> pChangeOutput = CreateChangeOutput(wallet, masterSeed, inputs, amount, fee);

	// Calculate total blinding excess sum for all inputs and outputs xS1 = xC - xI
	BlindingFactor totalBlindingExcessSum = CryptoUtil::AddBlindingFactors(&pChangeOutput->GetBlindingFactor(), pBlindingFactorSum.get());

	// Subtract random kernel offset oS from xS1. Calculate xS = xS1 - oS
	BlindingFactor transactionOffset = RandomNumberGenerator::GenerateRandom32();
	BlindingFactor secretKey = CryptoUtil::AddBlindingFactors(&totalBlindingExcessSum, &transactionOffset);
	std::unique_ptr<PublicKey> pPublicKey = Crypto::SECP256K1_CalculateCompressedPublicKey(secretKey);

	// Select a random nonce kS
	BlindingFactor secretNonce = *Crypto::GenerateSecureNonce();
	std::unique_ptr<PublicKey> pPublicNonce = Crypto::SECP256K1_CalculateCompressedPublicKey(secretNonce);

	// Build Transaction
	Transaction transaction = TransactionBuilder::BuildTransaction(inputs, *pChangeOutput, transactionOffset, fee, lockHeight);

	// Add values to Slate for passing to other participants: UUID, inputs, change_outputs, fee, amount, lock_height, kSG, xSG, oS
	std::unique_ptr<Slate> pSlate = std::make_unique<Slate>(Slate(SLATE_VERSION, 2, std::move(slateId), std::move(transaction), amount, fee, lockHeight, lockHeight));

	ParticipantData participantData(0, *pPublicKey, *pPublicNonce);
	// TODO: Add message signature
	pSlate->AddParticpantData(participantData);

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

	if (!SaveWalletTx(masterSeed, wallet, *pSlate, EWalletTxType::SENDING_IN_PROGRESS))
	{
		LoggerAPI::LogError("SlateBuilder::BuildSendSlate - Failed to create WalletTx");
	}

	return pSlate;
}

bool SlateBuilder::AddReceiverData(Wallet& wallet, const CBigInteger<32>& masterSeed, Slate& slate, const std::optional<std::string>& messageOpt) const
{
	// Slate was already received.
	if (wallet.GetSlateContext(slate.GetSlateId(), masterSeed) != nullptr)
	{
		return false;
	}

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
	std::unique_ptr<OutputData> pWalletCoin = wallet.CreateBlindedOutput(masterSeed, slate.GetAmount());
	BlindingFactor secretKey = pWalletCoin->GetBlindingFactor();
	std::unique_ptr<PublicKey> pPublicBlindExcess = Crypto::SECP256K1_CalculateCompressedPublicKey(secretKey);

	// Generate nonce
	BlindingFactor secretNonce = *Crypto::GenerateSecureNonce();
	std::unique_ptr<PublicKey> pPublicNonce = Crypto::SECP256K1_CalculateCompressedPublicKey(secretNonce);

	// Build receiver's ParticipantData
	ParticipantData receiverData(1, *pPublicBlindExcess, *pPublicNonce);

	// Generate signature
	std::vector<ParticipantData> participants = slate.GetParticipantData();
	participants.emplace_back(receiverData);

	std::unique_ptr<Signature> pPartialSignature = SignatureUtil::GeneratePartialSignature(secretKey, secretNonce, participants, message);
	if (pPartialSignature == nullptr)
	{
		LoggerAPI::LogError("SlateBuilder::AddReceiverData - Failed to generate signature for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	receiverData.AddPartialSignature(*pPartialSignature);

	// Add output to Transaction
	slate.UpdateTransaction(TransactionBuilder::AddOutput(slate.GetTransaction(), pWalletCoin->GetOutput()));

	// TODO: Add message signature

	// Add receiver's ParticipantData to Slate
	slate.AddParticpantData(receiverData);

	if (!SignatureUtil::VerifyPartialSignatures(slate.GetParticipantData(), message))
	{
		return false;
	}


	if (!SaveWalletTx(masterSeed, wallet, slate, EWalletTxType::RECEIVING_IN_PROGRESS))
	{
		LoggerAPI::LogError("SlateBuilder::BuildSendSlate - Failed to create WalletTx");
	}

	// Save secretKey and secretNonce
	if (!wallet.SaveSlateContext(slate.GetSlateId(), masterSeed, SlateContext(std::move(secretKey), std::move(secretNonce))))
	{
		LoggerAPI::LogError("SlateBuilder::AddReceiverData - Failed to save context for slate " + uuids::to_string(slate.GetSlateId()));
		return false;
	}

	return true;
}

std::unique_ptr<Transaction> SlateBuilder::Finalize(Wallet& wallet, const CBigInteger<32>& masterSeed, const Slate& slate) const
{
	// Generate message
	const Transaction& transaction = slate.GetTransaction();
	const std::vector<TransactionKernel>& kernels = transaction.GetBody().GetKernels();
	if (kernels.size() != 1)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const TransactionKernel& kernel = kernels.front();
	const Hash message = kernel.GetSignatureMessage();

	// Verify partial signatures
	if (!SignatureUtil::VerifyPartialSignatures(slate.GetParticipantData(), message))
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	// Load WalletTx
	std::unique_ptr<WalletTx> pWalletTx = wallet.GetTxBySlateId(masterSeed, slate.GetSlateId());
	if (pWalletTx == nullptr || pWalletTx->GetType() != EWalletTxType::SENDING_IN_PROGRESS)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	// Load secretKey and secretNonce
	std::unique_ptr<SlateContext> pSlateContext = wallet.GetSlateContext(slate.GetSlateId(), masterSeed);
	if (pSlateContext == nullptr)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const BlindingFactor& secretKey = pSlateContext->GetSecretKey();
	const BlindingFactor& secretNonce = pSlateContext->GetSecretNonce();

	// Generate partial signature
	std::unique_ptr<Signature> pPartialSignature = SignatureUtil::GeneratePartialSignature(secretKey, secretNonce, slate.GetParticipantData(), message);
	if (pPartialSignature == nullptr)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	std::vector<ParticipantData> participants = slate.GetParticipantData();
	for (ParticipantData& participant : participants)
	{
		if (participant.GetParticipantId() == 0)
		{
			participant.AddPartialSignature(*pPartialSignature);
			break;
		}
	}

	// Aggregate partial signatures
	std::unique_ptr<Signature> pAggSignature = SignatureUtil::AggregateSignatures(participants);
	if (pAggSignature == nullptr)
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	if (!SignatureUtil::VerifyAggregateSignature(*pAggSignature, participants, message))
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	// Build the final excess based on final tx and offset
	auto getInputCommitments = [](const TransactionInput& input) -> Commitment { return input.GetCommitment(); };
	std::vector<Commitment> inputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetInputs(), getInputCommitments);
	inputCommitments.emplace_back(*Crypto::CommitBlinded(0, transaction.GetOffset()));

	auto getOutputCommitments = [](const TransactionOutput& output) -> Commitment { return output.GetCommitment(); };
	std::vector<Commitment> outputCommitments = FunctionalUtil::map<std::vector<Commitment>>(transaction.GetBody().GetOutputs(), getOutputCommitments);
	outputCommitments.emplace_back(*Crypto::CommitTransparent(kernel.GetFee()));

	std::unique_ptr<Commitment> pFinalExcess = Crypto::AddCommitments(outputCommitments, inputCommitments);

	// Update the tx kernel to reflect the offset excess and sig
	TransactionKernel finalKernel(kernel.GetFeatures(), kernel.GetFee(), kernel.GetLockHeight(), Commitment(*pFinalExcess), Signature(*pAggSignature));
	if (!KernelSignatureValidator().VerifyKernelSignatures(std::vector<TransactionKernel>({ finalKernel })))
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	Transaction finalTransaction = TransactionBuilder::AddKernel(slate.GetTransaction(), finalKernel); // TODO: Shouldn't add kernel. Should replace existing kernel.
	if (!TransactionValidator().ValidateTransaction(finalTransaction))
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	// Update WalletTx
	pWalletTx->SetType(EWalletTxType::SENT);
	pWalletTx->SetTransaction(finalTransaction);
	if (!wallet.AddWalletTxs(masterSeed, std::vector<WalletTx>({ *pWalletTx })))
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	return std::make_unique<Transaction>(std::move(finalTransaction));
}

std::unique_ptr<OutputData> SlateBuilder::CreateChangeOutput(Wallet& wallet, const CBigInteger<32>& masterSeed, const std::vector<OutputData>& inputs, const uint64_t amount, const uint64_t fee) const
{
	uint64_t inputTotal = 0;
	for (const OutputData& input : inputs)
	{
		inputTotal += input.GetAmount();
	}
	const uint64_t changeAmount = inputTotal - (amount + fee);

	return wallet.CreateBlindedOutput(masterSeed, changeAmount);
}

bool SlateBuilder::SaveWalletTx(const CBigInteger<32>& masterSeed, Wallet& wallet, const Slate& slate, const EWalletTxType type) const
{
	WalletTx walletTx(
		wallet.GetNextWalletTxId(),
		type,
		std::make_optional<uuids::uuid>(uuids::uuid(slate.GetSlateId())),
		std::chrono::system_clock::now(),
		std::nullopt,
		(uint32_t)slate.GetTransaction().GetBody().GetInputs().size(),
		(uint32_t)slate.GetTransaction().GetBody().GetOutputs().size(),
		slate.GetAmount(), // TODO: Check EWalletTxType to determine credited vs debited
		0,
		std::optional<uint64_t>(slate.GetFee()),
		std::make_optional<Transaction>(Transaction(slate.GetTransaction())) // TODO: Should this be nullopt?
	);

	return wallet.AddWalletTxs(masterSeed, std::vector<WalletTx>({ walletTx }));
}