#include "WalletManagerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/ReceiveSlateBuilder.h"
#include "SlateBuilder/SendSlateBuilder.h"
#include "SlateBuilder/FinalizeSlateBuilder.h"
#include "SlateBuilder/CoinSelection.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/Exceptions/InvalidMnemonicException.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <thread>

WalletManager::WalletManager(const Config& config, INodeClient& nodeClient, IWalletDB* pWalletDB)
	: m_config(config), m_nodeClient(nodeClient), m_pWalletDB(pWalletDB), m_sessionManager(config, nodeClient, *pWalletDB)
{

}

WalletManager::~WalletManager()
{
	WalletDBAPI::CloseWalletDB(m_pWalletDB);
}

std::optional<std::pair<SecureString, SessionToken>> WalletManager::InitializeNewWallet(const std::string& username, const SecureString& password)
{
	LoggerAPI::LogInfo("Creating new wallet with username: " + username);
	const SecretKey walletSeed = RandomNumberGenerator::GenerateRandom32();
	const SecureVector walletSeedBytes(walletSeed.GetBytes().GetData().begin(), walletSeed.GetBytes().GetData().end());
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeedBytes, password);

	const std::string usernameLower = StringUtil::ToLower(username);
	if (m_pWalletDB->CreateWallet(usernameLower, encryptedSeed))
	{
		LoggerAPI::LogInfo("Wallet created with username: " + username);

		SecureString walletWords = Mnemonic::CreateMnemonic(walletSeed.GetBytes().GetData());
		SessionToken token = m_sessionManager.Login(usernameLower, walletSeedBytes);

		return std::make_optional<std::pair<SecureString, SessionToken>>(std::make_pair<SecureString, SessionToken>(std::move(walletWords), std::move(token)));
	}

	return std::nullopt;
}

std::optional<SessionToken> WalletManager::RestoreFromSeed(const std::string& username, const SecureString& password, const SecureString& walletWords)
{
	LoggerAPI::LogInfo("Attempting to restore account with username: " + username);
	std::optional<SecureVector> entropyOpt = Mnemonic::ToEntropy(walletWords);
	if (entropyOpt.has_value())
	{
		const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(entropyOpt.value(), password);
		const std::string usernameLower = StringUtil::ToLower(username);
		if (m_pWalletDB->CreateWallet(usernameLower, encryptedSeed))
		{
			LoggerAPI::LogInfo("Wallet restored for username: " + username);
			return std::make_optional<SessionToken>(m_sessionManager.Login(usernameLower, entropyOpt.value()));
		}
	}
	else
	{
		LoggerAPI::LogWarning("Mnemonic invalid for username: " + username);
		throw InvalidMnemonicException();
	}

	return std::nullopt;
}

bool WalletManager::CheckForOutputs(const SessionToken& token, const bool fromGenesis)
{
	try
	{
		const SecureVector masterSeed = m_sessionManager.GetSeed(token);
		LockedWallet wallet = m_sessionManager.GetWallet(token);

		wallet.GetWallet().RefreshOutputs(masterSeed, fromGenesis);
	}
	catch(const std::exception& e)
	{
		LoggerAPI::LogError("WalletManager::CheckForOutputs - Exception thrown: " + std::string(e.what()));
	}
	

	return true;
}

SecretKey WalletManager::GetGrinboxAddress(const SessionToken& token) const
{
	return m_sessionManager.GetGrinboxAddress(token);
}

std::vector<std::string> WalletManager::GetAllAccounts() const
{
	return m_pWalletDB->GetAccounts();
}

std::unique_ptr<SessionToken> WalletManager::Login(const std::string& username, const SecureString& password)
{
	LoggerAPI::LogInfo("Attempting to login with username: " + username);

	const std::string usernameLower = StringUtil::ToLower(username);
	std::unique_ptr<SessionToken> pToken = m_sessionManager.Login(usernameLower, password);
	if (pToken != nullptr)
	{
		LoggerAPI::LogInfo("Login successful for username: " + username);

		CheckForOutputs(*pToken, false);
	}

	return pToken;
}

void WalletManager::Logout(const SessionToken& token)
{
	m_sessionManager.Logout(token);
}

WalletSummary WalletManager::GetWalletSummary(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	return wallet.GetWallet().GetWalletSummary(masterSeed);
}

std::vector<WalletTxDTO> WalletManager::GetTransactions(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	std::vector<OutputData> outputs = m_pWalletDB->GetOutputs(wallet.GetWallet().GetUsername(), masterSeed);
	std::vector<WalletTx> walletTransactions = wallet.GetWallet().GetTransactions(masterSeed);

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

	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	std::vector<OutputData> outputs = m_pWalletDB->GetOutputs(wallet.GetWallet().GetUsername(), masterSeed);
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

uint64_t WalletManager::EstimateFee(const SessionToken& token, const uint64_t amountToSend, const uint64_t feeBase, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	// Select inputs using desired selection strategy.
	const uint8_t totalNumOutputs = numChangeOutputs + 1;
	const uint64_t numKernels = 1;
	const std::vector<OutputData> availableCoins = wallet.GetWallet().GetAllAvailableCoins(masterSeed);
	std::vector<OutputData> inputs = CoinSelection().SelectCoinsToSpend(availableCoins, amountToSend, feeBase, strategy, totalNumOutputs, numKernels);

	// Calculate the fee
	return WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), totalNumOutputs, numKernels);
}

std::unique_ptr<Slate> WalletManager::Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	return SendSlateBuilder(m_config, m_nodeClient).BuildSendSlate(wallet.GetWallet(), masterSeed, amount, feeBase, numChangeOutputs, messageOpt, strategy);
}

std::unique_ptr<Slate> WalletManager::Receive(const SessionToken& token, const Slate& slate, const std::optional<std::string>& messageOpt)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	return ReceiveSlateBuilder().AddReceiverData(wallet.GetWallet(), masterSeed, slate, messageOpt);
}

std::unique_ptr<Slate> WalletManager::Finalize(const SessionToken& token, const Slate& slate)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	return FinalizeSlateBuilder().Finalize(wallet.GetWallet(), masterSeed, slate);
}

bool WalletManager::PostTransaction(const SessionToken& token, const Transaction& transaction)
{
	return m_nodeClient.PostTransaction(transaction);
}

bool WalletManager::RepostByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	std::unique_ptr<WalletTx> pWalletTx = wallet.GetWallet().GetTxById(masterSeed, walletTxId);
	if (pWalletTx != nullptr)
	{
		if (pWalletTx->GetTransaction().has_value())
		{
			// TODO: Validate tx first, and make sure it's not already confirmed.
			return m_nodeClient.PostTransaction(pWalletTx->GetTransaction().value());
		}
	}

	return false;
}

bool WalletManager::CancelByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	std::unique_ptr<WalletTx> pWalletTx = wallet.GetWallet().GetTxById(masterSeed, walletTxId);
	if (pWalletTx != nullptr)
	{
		return wallet.GetWallet().CancelWalletTx(masterSeed, *pWalletTx);
	}

	return false;
}

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletManager* StartWalletManager(const Config& config, INodeClient& nodeClient)
	{
		IWalletDB* pWalletDB = WalletDBAPI::OpenWalletDB(config);
		return new WalletManager(config, nodeClient, pWalletDB);
	}

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletManager(IWalletManager* pWalletManager)
	{
		delete (WalletManager*)pWalletManager;
	}
}