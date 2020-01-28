#include "WalletManagerImpl.h"
#include "CancelTx.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/ReceiveSlateBuilder.h"
#include "SlateBuilder/SendSlateBuilder.h"
#include "SlateBuilder/FinalizeSlateBuilder.h"
#include "SlateBuilder/CoinSelection.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/Exceptions/InvalidMnemonicException.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Core/Exceptions/WalletException.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorManager.h>
#include <cassert>
#include <thread>

WalletManager::WalletManager(const Config& config, INodeClientPtr pNodeClient, std::shared_ptr<IWalletStore> pWalletStore)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletStore(pWalletStore),
	m_sessionManager(SessionManager::Create(config, pNodeClient, pWalletStore, *this))
{

}

std::pair<SecureString, SessionToken> WalletManager::InitializeNewWallet(const std::string& username, const SecureString& password)
{
	WALLET_INFO_F("Creating new wallet with username: {}", username);
	const SecretKey walletSeed = RandomNumberGenerator::GenerateRandom32();
	const SecureVector walletSeedBytes(walletSeed.GetVec().begin(), walletSeed.GetVec().end());
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeedBytes, password);
	SecureString walletWords = Mnemonic::CreateMnemonic(walletSeed.GetVec());

	const std::string usernameLower = StringUtil::ToLower(username);
	m_pWalletStore->CreateWallet(usernameLower, encryptedSeed);

	WALLET_INFO_F("Wallet created with username: {}", username);

	SessionToken token = m_sessionManager.Write()->Login(usernameLower, walletSeedBytes);

	return std::make_pair(std::move(walletWords), std::move(token));
}

std::optional<SessionToken> WalletManager::RestoreFromSeed(const std::string& username, const SecureString& password, const SecureString& walletWords)
{
	WALLET_INFO_F("Attempting to restore account with username: {}", username);
	try
	{
		SecureVector entropy = Mnemonic::ToEntropy(walletWords);

		const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(entropy, password);
		const std::string usernameLower = StringUtil::ToLower(username);

		m_pWalletStore->CreateWallet(usernameLower, encryptedSeed);

		WALLET_INFO_F("Wallet restored for username: {}", username);
		return std::make_optional(m_sessionManager.Write()->Login(usernameLower, entropy));
	}
	catch (std::exception& e)
	{
		WALLET_WARNING_F("Mnemonic invalid for username ({}). Error: {}", username, e.what());
		throw InvalidMnemonicException();
	}
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
		WALLET_ERROR_F("Exception thrown: {}", e.what());
		throw WALLET_EXCEPTION(e.what());
	}
}

SecretKey WalletManager::GetGrinboxAddress(const SessionToken& token) const
{
	SecureVector seed = m_sessionManager.Read()->GetSeed(token);
	KeyChain grinboxKeyChain = KeyChain::ForGrinbox(m_config, seed);

	return grinboxKeyChain.DerivePrivateKey(KeyChainPath(std::vector<uint32_t>({ 0 }))); // TODO: Determine KeyChainPath
}

std::optional<TorAddress> WalletManager::GetTorAddress(const SessionToken& token) const
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	return wallet.Read()->GetTorAddress();
}

std::optional<TorAddress> WalletManager::AddTorListener(const SessionToken& token, const KeyChainPath& path)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	KeyChain keyChain = KeyChain::FromSeed(m_config, m_sessionManager.Read()->GetSeed(token));
	SecretKey64 torKey = keyChain.DeriveED25519Key(path);

	std::shared_ptr<TorAddress> pTorAddress = 
		TorManager::GetInstance(m_config.GetTorConfig()).AddListener(torKey, wallet.Read()->GetListenerPort());
	if (pTorAddress != nullptr)
	{
		wallet.Write()->SetTorAddress(*pTorAddress);
	}

	return wallet.Read()->GetTorAddress();
}

uint16_t WalletManager::GetListenerPort(const SessionToken& token) const
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	return wallet.Read()->GetListenerPort();
}

std::vector<std::string> WalletManager::GetAllAccounts() const
{
	return m_pWalletStore->GetAccounts();
}

SessionToken WalletManager::Login(const std::string& username, const SecureString& password)
{
	WALLET_INFO_F("Attempting to login with username: {}", username);

	try
	{
		const std::string usernameLower = StringUtil::ToLower(username);
		SessionToken token = m_sessionManager.Write()->Login(usernameLower, password);

		WALLET_INFO_F("Login successful for username: {}", username);
		CheckForOutputs(token, false);

		return token;
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Error ({}) while attempting to login as: {}", e.what(), username);
		throw WALLET_EXCEPTION(e.what());
	}
}

void WalletManager::Logout(const SessionToken& token)
{
	m_sessionManager.Write()->Logout(token);
}

void WalletManager::DeleteWallet(const std::string& username, const SecureString& password)
{
	WALLET_INFO_F("Attempting to delete wallet with username: {}", username);

	try
	{
		const std::string usernameLower = StringUtil::ToLower(username);
		SessionToken token = m_sessionManager.Write()->Login(usernameLower, password);

		m_sessionManager.Write()->Logout(token);

		// TODO: Delete wallet folder.
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Error ({}) while attempting to delete wallet: {}", e.what(), username);
		throw WALLET_EXCEPTION(e.what());
	}
}

WalletSummaryDTO WalletManager::GetWalletSummary(const SessionToken& token)
{
	try
	{
		const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
		Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

		return wallet.Write()->GetWalletSummary(masterSeed);
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Exception occurred while building wallet summary: {}", e.what());
		throw WALLET_EXCEPTION(e.what());
	}
}

std::vector<WalletTxDTO> WalletManager::GetTransactions(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	auto pReader = wallet.Read()->GetDatabase().Read();
	std::vector<OutputDataEntity> outputs = pReader->GetOutputs(masterSeed);
	std::vector<WalletTx> walletTransactions = pReader->GetTransactions(masterSeed);

	std::vector<WalletTxDTO> walletTxDTOs;
	for (const WalletTx& walletTx : walletTransactions)
	{
		std::vector<WalletOutputDTO> outputDTOs;
		for (const OutputDataEntity& output : outputs)
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

	std::vector<OutputDataEntity> outputs = wallet.Read()->GetDatabase().Read()->GetOutputs(masterSeed);
	for (const OutputDataEntity& output : outputs)
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
	const std::vector<OutputDataEntity> availableCoins = wallet.Write()->GetAllAvailableCoins(masterSeed);
	std::vector<OutputDataEntity> inputs = CoinSelection().SelectCoinsToSpend(availableCoins, amountToSend, feeBase, strategy.GetStrategy(), strategy.GetInputs(), totalNumOutputs, numKernels);

	// Calculate the fee
	const uint64_t fee = WalletUtil::CalculateFee(feeBase, (int64_t)inputs.size(), totalNumOutputs, numKernels);

	std::vector<WalletOutputDTO> inputDTOs;
	for (const OutputDataEntity& input : inputs)
	{
		inputDTOs.emplace_back(WalletOutputDTO(input));
	}

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
		false, // TODO: Implement
		sendCriteria.GetAddress(),
		sendCriteria.GetMsg(),
		sendCriteria.GetSelectionStrategy(),
		sendCriteria.GetSlateVersion()
	);
}

Slate WalletManager::Receive(const ReceiveCriteria& receiveCriteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(receiveCriteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(receiveCriteria.GetToken());

	return ReceiveSlateBuilder(m_config).AddReceiverData(
		wallet,
		masterSeed,
		receiveCriteria.GetSlate(),
		receiveCriteria.GetAddress(),
		receiveCriteria.GetMsg()
	);
}

Slate WalletManager::Finalize(const FinalizeCriteria& finalizeCriteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(finalizeCriteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(finalizeCriteria.GetToken());

	Slate finalizedSlate = FinalizeSlateBuilder().Finalize(wallet, masterSeed, finalizeCriteria.GetSlate());
	if (finalizeCriteria.GetPostMethod().has_value())
	{
		PostTransaction(
			finalizedSlate.GetTransaction(),
			finalizeCriteria.GetPostMethod().value()
		);
	}

	return finalizedSlate;
}

bool WalletManager::PostTransaction(const Transaction& transaction, const PostMethodDTO& postMethod)
{
	const EPostMethod method = postMethod.GetMethod();
	if (method == EPostMethod::JOIN)
	{
		assert(postMethod.GetGrinJoinAddress().has_value());

		try
		{
			auto pTorConnection = TorManager::GetInstance(m_config.GetTorConfig()).Connect(postMethod.GetGrinJoinAddress().value());
			if (pTorConnection != nullptr)
			{
				Json::Value params;
				params["tx"] = transaction.ToJSON();
				RPC::Request receiveTxRequest = RPC::Request::BuildRequest("submit_tx", params);
				RPC::Response receiveTxResponse = pTorConnection->Invoke(receiveTxRequest, "/v1");

				return !receiveTxResponse.GetError().has_value();
			}
		}
		catch (std::exception& e)
		{
			WALLET_ERROR_F("Exception thrown: {}", e.what());
		}
	}
	else
	{
		const EPoolType poolType = (method == EPostMethod::FLUFF) ? EPoolType::MEMPOOL : EPoolType::STEMPOOL;

		return m_pNodeClient->PostTransaction(std::make_shared<Transaction>(transaction), poolType);
	}

	return false;
}

bool WalletManager::RepostByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	std::unique_ptr<WalletTx> pWalletTx = wallet.Read()->GetDatabase().Read()->GetTransactionById(masterSeed, walletTxId);
	if (pWalletTx != nullptr)
	{
		if (pWalletTx->GetTransaction().has_value())
		{
			return m_pNodeClient->PostTransaction(std::make_shared<Transaction>(pWalletTx->GetTransaction().value()), EPoolType::MEMPOOL);
		}
	}

	return false;
}

void WalletManager::CancelByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	std::unique_ptr<WalletTx> pWalletTx = wallet.Read()->GetDatabase().Read()->GetTransactionById(masterSeed, walletTxId);
	if (pWalletTx != nullptr)
	{
		CancelTx::CancelWalletTx(masterSeed, wallet.Write()->GetDatabase(), *pWalletTx);
	}
}

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletManagerPtr CreateWalletManager(const Config& config, INodeClientPtr pNodeClient)
	{
		std::shared_ptr<IWalletStore> pWalletDB = WalletDBAPI::OpenWalletDB(config);
		return std::shared_ptr<IWalletManager>(new WalletManager(config, pNodeClient, pWalletDB));
	}
}