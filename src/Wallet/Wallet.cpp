#include "Wallet.h"
#include "WalletRefresher.h"
#include <Wallet/Keychain/KeyChain.h>
#include <Infrastructure/Logger.h>
#include <Crypto/CryptoException.h>
#include <Core/Exceptions/WalletException.h>
#include <Consensus/Common.h>
#include <unordered_set>

Wallet::Wallet(const Config& config, INodeClientConstPtr pNodeClient, Locked<IWalletDB> walletDB, const std::string& username, KeyChainPath&& userPath)
	: m_config(config), m_pNodeClient(pNodeClient), m_walletDB(walletDB), m_username(username), m_userPath(std::move(userPath))
{

}

Locked<Wallet> Wallet::LoadWallet(const Config& config, INodeClientConstPtr pNodeClient, Locked<IWalletDB> walletDB, const std::string& username)
{
	KeyChainPath userPath = KeyChainPath::FromString("m/0/0"); // FUTURE: Support multiple account paths
	return Locked<Wallet>(std::make_shared<Wallet>(Wallet(config, pNodeClient, walletDB, username, std::move(userPath))));
}

WalletSummaryDTO Wallet::GetWalletSummary(const SecureVector& masterSeed)
{
	uint64_t awaitingConfirmation = 0;
	uint64_t immature = 0;
	uint64_t locked = 0;
	uint64_t spendable = 0;

	const uint64_t lastConfirmedHeight = m_pNodeClient->GetChainHeight();
	const std::vector<OutputDataEntity> outputs = RefreshOutputs(masterSeed, false);
	for (const OutputDataEntity& outputData : outputs)
	{
		const EOutputStatus status = outputData.GetStatus();
		if (status == EOutputStatus::LOCKED)
		{
			locked += outputData.GetAmount();
		}
		else if (status == EOutputStatus::SPENDABLE)
		{
			spendable += outputData.GetAmount();
		}
		else if (status == EOutputStatus::IMMATURE)
		{
			immature += outputData.GetAmount();
		}
		else if (status == EOutputStatus::NO_CONFIRMATIONS)
		{
			awaitingConfirmation += outputData.GetAmount();
		}
	}

	const uint64_t total = awaitingConfirmation + immature + spendable;
	std::vector<WalletTx> transactions = m_walletDB.Read()->GetTransactions(masterSeed);
	return WalletSummaryDTO(
		lastConfirmedHeight,
		m_config.GetWalletConfig().GetMinimumConfirmations(),
		total,
		awaitingConfirmation,
		immature,
		locked,
		spendable,
		std::move(transactions)
	);
}

std::vector<OutputDataEntity> Wallet::RefreshOutputs(const SecureVector& masterSeed, const bool fromGenesis)
{
	return WalletRefresher(m_config, m_pNodeClient).Refresh(masterSeed, m_walletDB, fromGenesis);
}

std::vector<OutputDataEntity> Wallet::GetAllAvailableCoins(const SecureVector& masterSeed)
{
	const KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);

	std::vector<OutputDataEntity> coins;

	std::vector<Commitment> commitments;
	const std::vector<OutputDataEntity> outputs = RefreshOutputs(masterSeed, false);
	for (const OutputDataEntity& output : outputs)
	{
		if (output.GetStatus() == EOutputStatus::SPENDABLE)
		{
			coins.emplace_back(output);
		}
	}

	return coins;
}

OutputDataEntity Wallet::CreateBlindedOutput(
	const SecureVector& masterSeed,
	const uint64_t amount,
	const KeyChainPath& keyChainPath,
	const uint32_t walletTxId,
	const EBulletproofType& bulletproofType,
	const std::optional<std::string>& messageOpt)
{
	const KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);

	SecretKey blindingFactor = keyChain.DerivePrivateKey(keyChainPath, amount);
	Commitment commitment = Crypto::CommitBlinded(amount, BlindingFactor(blindingFactor.GetBytes()));

	RangeProof rangeProof = keyChain.GenerateRangeProof(keyChainPath, amount, commitment, blindingFactor, bulletproofType);

	TransactionOutput transactionOutput(
		EOutputFeatures::DEFAULT_OUTPUT,
		std::move(commitment),
		std::move(rangeProof)
	);
			
	return OutputDataEntity(
		KeyChainPath(keyChainPath),
		std::move(blindingFactor),
		std::move(transactionOutput),
		amount,
		EOutputStatus::NO_CONFIRMATIONS,
		std::make_optional(walletTxId),
		messageOpt
	);
}

BuildCoinbaseResponse Wallet::CreateCoinbase(
	const SecureVector& masterSeed,
	const uint64_t fees,
	const std::optional<KeyChainPath>& keyChainPathOpt)
{
	const KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);

	auto pDatabase = m_walletDB.BatchWrite();

	const uint64_t amount = Consensus::REWARD + fees;
	const KeyChainPath keyChainPath = keyChainPathOpt.value_or(
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

	BlindingFactor txOffset(Hash::ValueOf(0));
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
		Crypto::Blake2b(serializer.GetBytes())
	);
	TransactionKernel kernel(
		EKernelFeatures::COINBASE_KERNEL,
		0,
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

std::unique_ptr<WalletTx> Wallet::GetTxById(const SecureVector& masterSeed, const uint32_t walletTxId) const
{
	std::vector<WalletTx> transactions = m_walletDB.Read()->GetTransactions(masterSeed);
	for (WalletTx& walletTx : transactions)
	{
		if (walletTx.GetId() == walletTxId)
		{
			return std::make_unique<WalletTx>(WalletTx(walletTx));
		}
	}

	WALLET_INFO_F("Could not find transaction {}", walletTxId);
	return std::unique_ptr<WalletTx>(nullptr);
}

std::unique_ptr<WalletTx> Wallet::GetTxBySlateId(const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	std::vector<WalletTx> transactions = m_walletDB.Read()->GetTransactions(masterSeed);
	for (WalletTx& walletTx : transactions)
	{
		if (walletTx.GetSlateId().has_value() && walletTx.GetSlateId().value() == slateId)
		{
			return std::make_unique<WalletTx>(WalletTx(walletTx));
		}
	}

	WALLET_INFO("Could not find transaction " + uuids::to_string(slateId));
	return std::unique_ptr<WalletTx>(nullptr);
}