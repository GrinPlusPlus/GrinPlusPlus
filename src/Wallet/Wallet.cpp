#include "Wallet.h"
#include "WalletRefresher.h"
#include "Keychain/KeyChain.h"
#include <Infrastructure/Logger.h>
#include <Crypto/CryptoException.h>
#include <Core/Exceptions/WalletException.h>
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

WalletSummary Wallet::GetWalletSummary(const SecureVector& masterSeed)
{
	uint64_t awaitingConfirmation = 0;
	uint64_t immature = 0;
	uint64_t locked = 0;
	uint64_t spendable = 0;

	const uint64_t lastConfirmedHeight = m_pNodeClient->GetChainHeight();
	const std::vector<OutputData> outputs = RefreshOutputs(masterSeed, false);
	for (const OutputData& outputData : outputs)
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
	return WalletSummary(
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

std::vector<OutputData> Wallet::RefreshOutputs(const SecureVector& masterSeed, const bool fromGenesis)
{
	return WalletRefresher(m_config, m_pNodeClient).Refresh(masterSeed, m_walletDB, fromGenesis);
}

std::vector<OutputData> Wallet::GetAllAvailableCoins(const SecureVector& masterSeed)
{
	const KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);

	std::vector<OutputData> coins;

	std::vector<Commitment> commitments;
	const std::vector<OutputData> outputs = RefreshOutputs(masterSeed, false);
	for (const OutputData& output : outputs)
	{
		if (output.GetStatus() == EOutputStatus::SPENDABLE)
		{
			coins.emplace_back(OutputData(output));
		}
	}

	return coins;
}

OutputData Wallet::CreateBlindedOutput(
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
			
	return OutputData(
		KeyChainPath(keyChainPath),
		std::move(blindingFactor),
		std::move(transactionOutput),
		amount,
		EOutputStatus::NO_CONFIRMATIONS,
		std::make_optional(walletTxId),
		messageOpt
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

	WALLET_INFO_F("Could not find transaction %lu", walletTxId);
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