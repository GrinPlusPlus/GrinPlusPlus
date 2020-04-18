#include "WalletManagerImpl.h"
#include "CancelTx.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/ReceiveSlateBuilder.h"
#include "SlateBuilder/SendSlateBuilder.h"
#include "SlateBuilder/FinalizeSlateBuilder.h"
#include "SlateBuilder/CoinSelection.h"
#include "WalletTxLoader.h"

#include <Wallet/WalletUtil.h>
#include <Wallet/Exceptions/InvalidMnemonicException.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Core/Exceptions/WalletException.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorProcess.h>
#include <cassert>
#include <thread>

WalletManager::WalletManager(const Config& config, INodeClientPtr pNodeClient, std::shared_ptr<IWalletStore> pWalletStore)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletStore(pWalletStore),
	m_sessionManager(SessionManager::Create(config, pNodeClient, pWalletStore, *this))
{

}

CreateWalletResponse WalletManager::InitializeNewWallet(
	const CreateWalletCriteria& criteria,
	const TorProcess::Ptr& pTorProcess)
{
	WALLET_INFO_F(
		"Creating new wallet with username {} and numWords {}",
		criteria.GetUsername(),
		criteria.GetNumWords()
	);

	const size_t entropyBytes = (4 * criteria.GetNumWords()) / 3;
	const SecureVector walletSeed = RandomNumberGenerator::GenerateRandomBytes(entropyBytes);
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, criteria.GetPassword());
	SecureString walletWords = Mnemonic::CreateMnemonic((const std::vector<uint8_t>&)walletSeed);

	m_pWalletStore->CreateWallet(criteria.GetUsername(), encryptedSeed);

	WALLET_INFO_F("Wallet created with username: {}", criteria.GetUsername());

	SessionToken token = m_sessionManager.Write()->Login(
		pTorProcess,
		criteria.GetUsername(),
		walletSeed
	);

	auto wallet = m_sessionManager.Read()->GetWallet(token);
	const uint16_t listenerPort = wallet.Read()->GetListenerPort();
	std::optional<TorAddress> torAddressOpt = wallet.Read()->GetTorAddress();

	return CreateWalletResponse(token, listenerPort, walletWords, torAddressOpt);
}

LoginResponse WalletManager::RestoreFromSeed(
	const RestoreWalletCriteria& criteria,
	const TorProcess::Ptr& pTorProcess)
{
	WALLET_INFO_F("Attempting to restore account with username: {}", criteria.GetUsername());
	try
	{
		SecureVector entropy = Mnemonic::ToEntropy(criteria.GetSeedWords());

		const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(entropy, criteria.GetPassword());

		m_pWalletStore->CreateWallet(criteria.GetUsername(), encryptedSeed);

		WALLET_INFO_F("Wallet restored for username: {}", criteria.GetUsername());
		SessionToken token = m_sessionManager.Write()->Login(pTorProcess, criteria.GetUsername(), entropy);

		auto wallet = m_sessionManager.Read()->GetWallet(token);
		const uint16_t listenerPort = wallet.Read()->GetListenerPort();
		std::optional<TorAddress> torAddressOpt = wallet.Read()->GetTorAddress();

		return LoginResponse(token, listenerPort, torAddressOpt);
	}
	catch (std::exception& e)
	{
		WALLET_WARNING_F("Mnemonic invalid for username ({}). Error: {}", criteria.GetUsername(), e.what());
		throw InvalidMnemonicException();
	}
}

SecureString WalletManager::GetSeedWords(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	return Mnemonic::CreateMnemonic((const std::vector<unsigned char>&)masterSeed);
}

SecureString WalletManager::GetSeedWords(const GetSeedPhraseCriteria& criteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(
		criteria.GetUsername(),
		criteria.GetPassword()
	);
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

std::optional<TorAddress> WalletManager::GetTorAddress(const SessionToken& token) const
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	return wallet.Read()->GetTorAddress();
}

std::optional<TorAddress> WalletManager::AddTorListener(const SessionToken& token, const KeyChainPath& path, const TorProcess::Ptr& pTorProcess)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);

	KeyChain keyChain = KeyChain::FromSeed(m_config, m_sessionManager.Read()->GetSeed(token));
	SecretKey64 torKey = keyChain.DeriveED25519Key(path);

	std::shared_ptr<TorAddress> pTorAddress = pTorProcess->AddListener(torKey, wallet.Read()->GetListenerPort());
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

LoginResponse WalletManager::Login(
	const LoginCriteria& criteria,
	const TorProcess::Ptr& pTorProcess)
{
	WALLET_INFO_F("Attempting to login with username: {}", criteria.GetUsername());

	try
	{
		SessionToken token = m_sessionManager.Write()->Login(
			pTorProcess,
			criteria.GetUsername(),
			criteria.GetPassword()
		);

		WALLET_INFO_F("Login successful for username: {}", criteria.GetUsername());
		CheckForOutputs(token, false);
		auto wallet = m_sessionManager.Read()->GetWallet(token);
		const uint16_t listenerPort = wallet.Read()->GetListenerPort();
		std::optional<TorAddress> torAddressOpt = wallet.Read()->GetTorAddress();

		return LoginResponse(token, listenerPort, torAddressOpt);
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Error ({}) while attempting to login as: {}", e.what(), criteria.GetUsername());
		throw;
	}
}

void WalletManager::Logout(const SessionToken& token)
{
	m_sessionManager.Write()->Logout(token);
}

void WalletManager::DeleteWallet(const std::string& username, const SecureString& password)
{
	WALLET_WARNING_F("Attempting to delete wallet with username: {}", username);

	try
	{
		const std::string usernameLower = StringUtil::ToLower(username);
		m_sessionManager.Read()->Authenticate(usernameLower, password);

		m_pWalletStore->DeleteWallet(usernameLower);
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Error ({}) while attempting to delete wallet: {}", e.what(), username);
		throw WALLET_EXCEPTION(e.what());
	}
}

void WalletManager::ChangePassword(
	const std::string& username,
	const SecureString& currentPassword,
	const SecureString& newPassword)
{
	const std::string usernameLower = StringUtil::ToLower(username);
	EncryptedSeed encrypted = m_pWalletStore->LoadWalletSeed(usernameLower);
	SecureVector walletSeed = SeedEncrypter().DecryptWalletSeed(encrypted, currentPassword);

	// Valid password - Re-encrypt seed
	EncryptedSeed newSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, newPassword);
	m_pWalletStore->ChangePassword(usernameLower, newSeed);
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

std::vector<WalletTxDTO> WalletManager::GetTransactions(const ListTxsCriteria& criteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(criteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(criteria.GetToken());

	auto pReader = wallet.Read()->GetDatabase().Read();
	return WalletTxLoader().LoadTransactions(pReader.GetShared(), masterSeed, criteria);
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
	std::vector<OutputDataEntity> inputs = CoinSelection().SelectCoinsToSpend(
		availableCoins,
		amountToSend,
		feeBase,
		strategy.GetStrategy(),
		strategy.GetInputs(),
		totalNumOutputs,
		numKernels
	);

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

Slate WalletManager::Finalize(const FinalizeCriteria& finalizeCriteria, const TorProcess::Ptr& pTorProcess)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(finalizeCriteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(finalizeCriteria.GetToken());

	Slate finalizedSlate = FinalizeSlateBuilder().Finalize(
		wallet,
		masterSeed,
		finalizeCriteria.GetSlate()
	);
	if (finalizeCriteria.GetPostMethod().has_value())
	{
		PostTransaction(
			finalizedSlate.GetTransaction(),
			finalizeCriteria.GetPostMethod().value(),
			pTorProcess
		);
	}

	return finalizedSlate;
}

bool WalletManager::PostTransaction(
	const Transaction& transaction,
	const PostMethodDTO& postMethod,
	const TorProcess::Ptr& pTorProcess)
{
	const EPostMethod method = postMethod.GetMethod();
	if (method == EPostMethod::JOIN)
	{
		assert(postMethod.GetGrinJoinAddress().has_value());

		try
		{
			auto pTorConnection = pTorProcess->Connect(postMethod.GetGrinJoinAddress().value());
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
			return m_pNodeClient->PostTransaction(
				std::make_shared<Transaction>(pWalletTx->GetTransaction().value()),
				EPoolType::MEMPOOL
			);
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

BuildCoinbaseResponse WalletManager::BuildCoinbase(const BuildCoinbaseCriteria& criteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(criteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(criteria.GetToken());

	return wallet.Write()->CreateCoinbase(masterSeed, criteria.GetFees(), criteria.GetPath());
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