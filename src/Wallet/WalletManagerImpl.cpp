#include "WalletManagerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/ReceiveSlateBuilder.h"
#include "SlateBuilder/SendSlateBuilder.h"
#include "SlateBuilder/FinalizeSlateBuilder.h"
#include "SlateBuilder/CoinSelection.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/Exceptions/InvalidMnemonicException.h>
#include <Core/Exceptions/WalletException.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <thread>

WalletManager::WalletManager(const Config& config, INodeClientPtr pNodeClient, IWalletDBPtr pWalletDB)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletDB(pWalletDB),
	m_sessionManager(SessionManager::Create(config, pNodeClient, pWalletDB, *this))
{

}

std::optional<std::pair<SecureString, SessionToken>> WalletManager::InitializeNewWallet(const std::string& username, const SecureString& password)
{
	WALLET_INFO_F("Creating new wallet with username: %s", username);
	const SecretKey walletSeed = RandomNumberGenerator::GenerateRandom32();
	const SecureVector walletSeedBytes(walletSeed.GetBytes().GetData().begin(), walletSeed.GetBytes().GetData().end());
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeedBytes, password);
	SecureString walletWords = Mnemonic::CreateMnemonic(walletSeed.GetBytes().GetData());

	const std::string usernameLower = StringUtil::ToLower(username);
	if (m_pWalletDB->CreateWallet(usernameLower, encryptedSeed))
	{
		WALLET_INFO_F("Wallet created with username: %s", username);

		SessionToken token = m_sessionManager.Write()->Login(usernameLower, walletSeedBytes);

		return std::make_optional(std::make_pair(std::move(walletWords), std::move(token)));
	}

	return std::nullopt;
}

std::optional<SessionToken> WalletManager::RestoreFromSeed(const std::string& username, const SecureString& password, const SecureString& walletWords)
{
	WALLET_INFO_F("Attempting to restore account with username: %s", username);
	try
	{
		SecureVector entropy = Mnemonic::ToEntropy(walletWords);

		const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(entropy, password);
		const std::string usernameLower = StringUtil::ToLower(username);
		if (m_pWalletDB->CreateWallet(usernameLower, encryptedSeed))
		{
			WALLET_INFO_F("Wallet restored for username: %s", username);
			return std::make_optional(m_sessionManager.Write()->Login(usernameLower, entropy));
		}
	}
	catch (std::exception& e)
	{
		WALLET_WARNING_F("Mnemonic invalid for username (%s). Error: %s", username, e.what());
		throw InvalidMnemonicException();
	}

	return std::nullopt;
}

SecureString WalletManager::GetSeedWords(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	return Mnemonic::CreateMnemonic((const std::vector<unsigned char>&)masterSeed);
}

void WalletManager::CheckForOutputs(const SessionToken& token, const bool fromGenesis)
{
	try
	{
		const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
		Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

		wallet.Write()->RefreshOutputs(masterSeed, fromGenesis);
	}
	catch(const std::exception& e)
	{
		WALLET_ERROR_F("Exception thrown: %s", e.what());
		throw WALLET_EXCEPTION(e.what());
	}
}

SecretKey WalletManager::GetGrinboxAddress(const SessionToken& token) const
{
	return m_sessionManager.Read()->GetGrinboxAddress(token);
}

std::optional<TorAddress> WalletManager::GetTorAddress(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	return wallet.Read()->GetTorAddress();
}

std::vector<std::string> WalletManager::GetAllAccounts() const
{
	return m_pWalletDB->GetAccounts();
}

SessionToken WalletManager::Login(const std::string& username, const SecureString& password)
{
	WALLET_INFO_F("Attempting to login with username: %s", username);

	try
	{
		const std::string usernameLower = StringUtil::ToLower(username);
		SessionToken token = m_sessionManager.Write()->Login(usernameLower, password);

		WALLET_INFO_F("Login successful for username: %s", username);
		CheckForOutputs(token, false);

		return token;
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Error (%s) while attempting to login as: %s", e.what(), username);
		throw WALLET_EXCEPTION(e.what());
	}
}

void WalletManager::Logout(const SessionToken& token)
{
	m_sessionManager.Write()->Logout(token);
}

void WalletManager::DeleteWallet(const std::string& username, const SecureString& password)
{
	WALLET_INFO_F("Attempting to delete wallet with username: %s", username);

	try
	{
		const std::string usernameLower = StringUtil::ToLower(username);
		SessionToken token = m_sessionManager.Write()->Login(usernameLower, password);

		m_sessionManager.Write()->Logout(token);

		// TODO: Delete wallet folder.
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Error (%s) while attempting to delete wallet: %s", e.what(), username);
		throw WALLET_EXCEPTION(e.what());
	}
}

WalletSummary WalletManager::GetWalletSummary(const SessionToken& token)
{
	try
	{
		const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
		Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

		return wallet.Write()->GetWalletSummary(masterSeed);
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Exception occurred while building wallet summary: %s", e.what());
		throw WALLET_EXCEPTION(e.what());
	}
}

std::vector<WalletTxDTO> WalletManager::GetTransactions(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	std::vector<OutputData> outputs = m_pWalletDB->GetOutputs(wallet.Read()->GetUsername(), masterSeed);
	std::vector<WalletTx> walletTransactions = wallet.Write()->GetTransactions(masterSeed);

	std::vector<WalletTxDTO> walletTxDTOs;
	for (const WalletTx& walletTx : walletTransactions)
	{
		std::vector<WalletOutputDTO> outputDTOs;
		for (const OutputData& output : outputs)
		{
			if (output.GetWalletTxId().has_value() && output.GetWalletTxId().value() == walletTx.GetId())
			{
				outputDTOs.emplace_back(WalletOutputDTO(output));
			}
		}

		walletTxDTOs.emplace_back(WalletTxDTO(walletTx, outputDTOs));
	}

	return walletTxDTOs;
}

std::vector<WalletOutputDTO> WalletManager::GetOutputs(const SessionToken& token, const bool includeSpent, const bool includeCanceled)
{
	std::vector<WalletOutputDTO> outputDTOs;

	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	std::vector<OutputData> outputs = m_pWalletDB->GetOutputs(wallet.Read()->GetUsername(), masterSeed);
	for (const OutputData& output : outputs)
	{
		if (output.GetStatus() == EOutputStatus::SPENT && !includeSpent)
		{
			continue;
		}

		if (output.GetStatus() == EOutputStatus::CANCELED && !includeCanceled)
		{
			continue;
		}

		outputDTOs.emplace_back(WalletOutputDTO(output));
	}

	return outputDTOs;
}

FeeEstimateDTO WalletManager::EstimateFee(
	const SessionToken& token, 
	const uint64_t amountToSend, 
	const uint64_t feeBase, 
	const SelectionStrategyDTO& strategy, 
	const uint8_t numChangeOutputs)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	// Select inputs using desired selection strategy.
	const uint8_t totalNumOutputs = numChangeOutputs + 1;
	const uint64_t numKernels = 1;
	const std::vector<OutputData> availableCoins = wallet.Write()->GetAllAvailableCoins(masterSeed);
	std::vector<OutputData> inputs = CoinSelection().SelectCoinsToSpend(availableCoins, amountToSend, feeBase, strategy.GetStrategy(), strategy.GetInputs(), totalNumOutputs, numKernels);
	
	std::vector<WalletOutputDTO> inputDTOs;
	for (const OutputData& input : inputs)
	{
		inputDTOs.emplace_back(WalletOutputDTO(input));
	}

	// Calculate the fee
	const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), totalNumOutputs, numKernels);

	return FeeEstimateDTO(fee, std::move(inputDTOs));
}

Slate WalletManager::Send(const SendCriteria& sendCriteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(sendCriteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(sendCriteria.GetToken());

	return SendSlateBuilder(m_config, m_pNodeClient).BuildSendSlate(
		wallet,
		masterSeed,
		sendCriteria.GetAmount(),
		sendCriteria.GetFeeBase(),
		sendCriteria.GetNumOutputs(),
		sendCriteria.GetAddress(),
		sendCriteria.GetMsg(),
		sendCriteria.GetSelectionStrategy()
	);
}

Slate WalletManager::Receive(
	const SessionToken& token,
	const Slate& slate,
	const std::optional<std::string>& addressOpt,
	const std::optional<std::string>& messageOpt)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	return ReceiveSlateBuilder().AddReceiverData(wallet, masterSeed, slate, addressOpt, messageOpt);
}

Slate WalletManager::Finalize(const SessionToken& token, const Slate& slate)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	return FinalizeSlateBuilder().Finalize(wallet, masterSeed, slate);
}

bool WalletManager::PostTransaction(const SessionToken& token, const Transaction& transaction)
{
	return m_pNodeClient->PostTransaction(transaction);
}

bool WalletManager::RepostByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	std::unique_ptr<WalletTx> pWalletTx = wallet.Write()->GetTxById(masterSeed, walletTxId);
	if (pWalletTx != nullptr)
	{
		if (pWalletTx->GetTransaction().has_value())
		{
			return m_pNodeClient->PostTransaction(pWalletTx->GetTransaction().value());
		}
	}

	return false;
}

bool WalletManager::CancelByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	std::unique_ptr<WalletTx> pWalletTx = wallet.Write()->GetTxById(masterSeed, walletTxId);
	if (pWalletTx != nullptr)
	{
		return wallet.Write()->CancelWalletTx(masterSeed, *pWalletTx);
	}

	return false;
}

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletManagerPtr CreateWalletManager(const Config& config, INodeClientPtr pNodeClient)
	{
		std::shared_ptr<IWalletDB> pWalletDB = WalletDBAPI::OpenWalletDB(config);
		return std::shared_ptr<IWalletManager>(new WalletManager(config, pNodeClient, pWalletDB));
	}
}