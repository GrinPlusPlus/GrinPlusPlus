#include <Wallet/Wallet.h>
#include <Consensus.h>
#include <Wallet/Keychain/KeyChain.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>
#include <Core/Exceptions/WalletException.h>
#include <Core/Util/FeeUtil.h>
#include <Crypto/Hasher.h>
#include <Crypto/Curve25519.h>

#include "CancelTx.h"
#include "WalletTxLoader.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/CoinSelection.h"

SecureString Wallet::GetSeedWords() const
{
	return Mnemonic::CreateMnemonic(m_master_seed.data(), m_master_seed.size());
}

WalletSummaryDTO Wallet::GetWalletSummary() const
{
	WalletBalanceDTO balance = GetBalance();
	
	std::vector<WalletTx> transactions = m_walletDB.Read()->GetTransactions(m_master_seed);
	return WalletSummaryDTO(
		m_pConfig->GetMinimumConfirmations(),
		balance,
		std::move(transactions)
	);
}

WalletBalanceDTO Wallet::GetBalance() const
{
	uint64_t awaitingConfirmation = 0;
	uint64_t immature = 0;
	uint64_t locked = 0;
	uint64_t spendable = 0;

    auto pWalletDB = m_walletDB.Read();
	const std::vector<OutputDataEntity> outputs = pWalletDB->GetOutputs(m_master_seed);
	for (const OutputDataEntity& outputData : outputs)
	{
		const EOutputStatus status = outputData.GetStatus();
		if (status == EOutputStatus::LOCKED) {
			locked += outputData.GetAmount();
		} else if (status == EOutputStatus::SPENDABLE) {
			spendable += outputData.GetAmount();
		} else if (status == EOutputStatus::IMMATURE) {
			immature += outputData.GetAmount();
		} else if (status == EOutputStatus::NO_CONFIRMATIONS) {
			awaitingConfirmation += outputData.GetAmount();
		}
	}

	return WalletBalanceDTO(
        pWalletDB->GetRefreshBlockHeight(),
		awaitingConfirmation,
		immature,
		locked,
		spendable
	);
}

std::unique_ptr<WalletTx> Wallet::GetTransactionById(const uint32_t txId) const
{
	return m_walletDB.Read()->GetTransactionById(m_master_seed, txId);
}

WalletTx Wallet::GetTransactionBySlateId(const uuids::uuid& slateId, const EWalletTxType type) const
{
	std::vector<WalletTx> txs = m_walletDB.Read()->GetTransactions(m_master_seed);
	for (const WalletTx& tx : txs)
	{
		if (tx.GetSlateId().has_value() && tx.GetSlateId().value() == slateId && tx.GetType() == type) {
			return tx;
		}
	}

	const std::string errorMsg = StringUtil::Format("Transaction not found for {}", uuids::to_string(slateId));
	WALLET_ERROR(errorMsg);
	throw WALLET_EXCEPTION(errorMsg);
}

std::vector<WalletTxDTO> Wallet::GetTransactions(const ListTxsCriteria& criteria) const
{
	return WalletTxLoader().LoadTransactions(m_walletDB.Read().GetShared(), m_master_seed, criteria);
}

std::vector<WalletOutputDTO> Wallet::GetOutputs(const bool includeSpent, const bool includeCanceled) const
{
    std::vector<WalletOutputDTO> filtered_outputs;

	const std::vector<OutputDataEntity> all_outputs = m_walletDB.Read()->GetOutputs(m_master_seed);
	for (const OutputDataEntity& output_data : all_outputs)
	{
		const EOutputStatus status = output_data.GetStatus();
        if (status == EOutputStatus::SPENT && !includeSpent) {
            continue;
        } else if (status == EOutputStatus::CANCELED && !includeCanceled) {
            continue;
        }

        filtered_outputs.push_back(WalletOutputDTO::FromOutputData(output_data));
    }

    return filtered_outputs;
}

Slate Wallet::GetSlate(const uuids::uuid& slateId, const SlateStage& stage) const
{
	auto pSlate = m_walletDB.Read()->LoadSlate(m_master_seed, slateId, stage);
	if (pSlate == nullptr) {
		WALLET_ERROR_F(
			"Failed to load slate for {}",
			uuids::to_string(slateId)
		);
		throw WALLET_EXCEPTION("Failed to load slate.");
	}

	return *pSlate;
}

SlateContextEntity Wallet::GetSlateContext(const uuids::uuid& slateId) const
{
	auto pSlateContext = m_walletDB.Read()->LoadSlateContext(m_master_seed, slateId);
	if (pSlateContext == nullptr) {
		WALLET_ERROR_F(
			"Failed to load slate context for {}",
			uuids::to_string(slateId)
		);
		throw WALLET_EXCEPTION("Failed to load slate context.");
	}

	return *pSlateContext;
}

std::optional<TorAddress> Wallet::AddTorListener(const KeyChainPath& path, const TorProcess::Ptr& pTorProcess)
{
	KeyChain keyChain = KeyChain::FromSeed(*m_pConfig, m_master_seed);
	ed25519_keypair_t torKey = keyChain.DeriveED25519Key(path);

	TorAddress tor_address = pTorProcess->AddListener(torKey.secret_key, GetListenerPort());
	SetTorAddress(tor_address);

	return GetTorAddress();
}

FeeEstimateDTO Wallet::EstimateFee(const EstimateFeeCriteria& criteria) const
{
	// Select inputs using desired selection strategy.
	const uint8_t totalNumOutputs = criteria.GetNumChangeOutputs() + 1;
	const uint64_t numKernels = 1;
	std::vector<OutputDataEntity> all_outputs = m_walletDB.Read()->GetOutputs(m_master_seed);
    std::vector<OutputDataEntity> available_coins;
    std::copy_if(
        all_outputs.cbegin(), all_outputs.cend(),
        std::back_inserter(available_coins),
        [](const OutputDataEntity& output_data) { return output_data.GetStatus() == EOutputStatus::SPENDABLE; }
    );

	std::vector<OutputDataEntity> inputs = available_coins;
	
	if (!criteria.SendEntireBalance()) {
		inputs = CoinSelection().SelectCoinsToSpend(
			available_coins,
			criteria.GetAmount(),
			criteria.GetFeeBase(),
			criteria.GetSelectionStrategy().GetStrategy(),
			criteria.GetSelectionStrategy().GetInputs(),
			totalNumOutputs,
			numKernels
		);
	}

	// Calculate the fee
	const uint64_t fee = FeeUtil::CalculateFee(
		criteria.GetFeeBase(),
		(int64_t)inputs.size(),
		(int64_t)totalNumOutputs,
		(int64_t)numKernels
	);

	std::vector<WalletOutputDTO> inputDTOs;
    std::transform(
        inputs.cbegin(), inputs.cend(),
        std::back_inserter(inputDTOs),
        [](const OutputDataEntity& input) { return WalletOutputDTO::FromOutputData(input); }
    );

	uint64_t amount = criteria.GetAmount();
	if (criteria.SendEntireBalance()) {
		const uint64_t total_amount = std::accumulate(
			available_coins.cbegin(), available_coins.cend(), (uint64_t)0,
			[](const uint64_t total, const OutputDataEntity& coin) { return total + coin.GetAmount(); }
		);

		if (total_amount < fee) {
			throw InsufficientFundsException();
		}

		amount = total_amount - fee;
	}

	return FeeEstimateDTO(amount, fee, std::move(inputDTOs));
}

// //
// // Sends coins to the given destination using the specified method. Returns a valid slate if successful.
// // Exceptions thrown:
// // * SessionTokenException - If no matching session found, or if the token is invalid.
// // * InsufficientFundsException - If there are not enough funds ready to spend after calculating and including the fee.
// //
// Slate Wallet::Send(const SendCriteria& sendCriteria)
// {

// }

// //
// // Receives coins and builds a received slate to return to the sender.
// //
// Slate Wallet::Receive(const ReceiveCriteria& receiveCriteria)
// {

// }

// Slate Wallet::Finalize(const Slate& slate, const std::optional<SlatepackMessage>& slatepackOpt)
// {

// }

void Wallet::CancelTx(const uint32_t walletTxId)
{
    auto pBatch = m_walletDB.BatchWrite();
	std::unique_ptr<WalletTx> pWalletTx = pBatch->GetTransactionById(m_master_seed, walletTxId);
	if (pWalletTx != nullptr) {
		CancelTx::CancelWalletTx(m_master_seed, pBatch.GetShared(), *pWalletTx);
        pBatch->Commit();
	}
}

BuildCoinbaseResponse Wallet::BuildCoinbase(const BuildCoinbaseCriteria& criteria)
{
    const KeyChain keyChain = KeyChain::FromSeed(*m_pConfig, m_master_seed);

	auto pDatabase = m_walletDB.BatchWrite();

	const uint64_t amount = Consensus::REWARD + criteria.GetFees();
	KeyChainPath keyChainPath = criteria.GetPath().value_or(
		pDatabase->GetNextChildPath(m_userPath)
	);
	SecretKey blindingFactor = keyChain.DerivePrivateKey(keyChainPath, amount);
	Commitment commitment = Crypto::CommitBlinded(
		amount,
		BlindingFactor(blindingFactor.GetBytes())
	);

	RangeProof rangeProof = keyChain.GenerateRangeProof(
		keyChainPath,
		amount,
		commitment,
		blindingFactor,
		EBulletproofType::ENHANCED
	);

	BlindingFactor txOffset;
	Commitment kernelCommitment = Crypto::AddCommitments(
		{ commitment },
		{ Crypto::CommitTransparent(amount) }
	);

	TransactionOutput output(
		EOutputFeatures::COINBASE_OUTPUT,
		std::move(commitment),
		std::move(rangeProof)
	);

	Serializer serializer;
	serializer.Append<uint8_t>((uint8_t)EKernelFeatures::COINBASE_KERNEL);

	auto pSignature = Crypto::BuildCoinbaseSignature(
		blindingFactor,
		kernelCommitment,
		Hasher::Blake2b(serializer.GetBytes())
	);
	TransactionKernel kernel(
		EKernelFeatures::COINBASE_KERNEL,
		Fee(),
		0,
		std::move(kernelCommitment),
		Signature(*pSignature)
	);

	pDatabase->Commit();

	return BuildCoinbaseResponse(
		std::move(kernel),
		std::move(output),
		std::move(keyChainPath)
	);
}

SlatepackMessage Wallet::DecryptSlatepack(const std::string& armoredSlatepack) const
{
	KeyChain keychain = KeyChain::FromSeed(*m_pConfig, m_master_seed);
	ed25519_keypair_t decrypt_key = keychain.DeriveED25519Key(KeyChainPath::FromString("m/0/1/0"));
	
	return Armor::Unpack(armoredSlatepack, Curve25519::ToX25519(decrypt_key));
}