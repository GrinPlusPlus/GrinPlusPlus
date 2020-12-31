#include "WalletManagerImpl.h"
#include "CancelTx.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/ReceiveSlateBuilder.h"
#include "SlateBuilder/SendSlateBuilder.h"
#include "SlateBuilder/FinalizeSlateBuilder.h"
#include "SlateBuilder/CoinSelection.h"
#include "WalletTxLoader.h"

#include <Wallet/Exceptions/InvalidMnemonicException.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Core/Exceptions/WalletException.h>
#include <Core/Util/FeeUtil.h>
#include <Crypto/CSPRNG.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>
#include <Net/Tor/TorConnection.h>
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

WalletManager::~WalletManager()
{
	LOG_INFO("Shutting down wallet manager");
}

Locked<Wallet> WalletManager::GetWallet(const SessionToken& token)
{
	SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
	m_sessionManager.Read()->GetWalletImpl(token).Write()->RefreshOutputs(masterSeed, false);

	return m_sessionManager.Read()->GetWallet(token);
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
	const SecureVector walletSeed = CSPRNG::GenerateRandomBytes(entropyBytes);
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, criteria.GetPassword());
	SecureString walletWords = Mnemonic::CreateMnemonic(walletSeed.data(), walletSeed.size());

	m_pWalletStore->CreateWallet(criteria.GetUsername(), encryptedSeed);

	WALLET_INFO_F("Wallet created with username: {}", criteria.GetUsername());

	SessionToken token = m_sessionManager.Write()->Login(
		pTorProcess,
		criteria.GetUsername(),
		walletSeed
	);

	auto pWallet = m_sessionManager.Read()->GetWallet(token).Read();
	return CreateWalletResponse(
		token,
		pWallet->GetListenerPort(),
		walletWords,
		pWallet->GetSlatepackAddress(),
		pWallet->GetTorAddress()
	);
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

		auto pWallet = m_sessionManager.Read()->GetWallet(token).Read();
		return LoginResponse(
			token,
			pWallet->GetListenerPort(),
			pWallet->GetSlatepackAddress(),
			pWallet->GetTorAddress()
		);
	}
	catch (std::exception& e)
	{
		WALLET_WARNING_F("Mnemonic invalid for username ({}). Error: {}", criteria.GetUsername(), e.what());
		throw InvalidMnemonicException();
	}
}

SecureString WalletManager::GetSeedWords(const GetSeedPhraseCriteria& criteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(
		criteria.GetUsername(),
		criteria.GetPassword()
	);
	return Mnemonic::CreateMnemonic(masterSeed.data(), masterSeed.size());
}

void WalletManager::CheckForOutputs(const SessionToken& token, const bool fromGenesis)
{
	try
	{
		const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(token);
		Locked<WalletImpl> wallet = m_sessionManager.Read()->GetWalletImpl(token);

		wallet.Write()->RefreshOutputs(masterSeed, fromGenesis);
	}
	catch(const std::exception& e)
	{
		WALLET_ERROR_F("Exception thrown: {}", e.what());
		throw WALLET_EXCEPTION(e.what());
	}
}

std::optional<TorAddress> WalletManager::AddTorListener(const SessionToken& token, const KeyChainPath& path, const TorProcess::Ptr& pTorProcess)
{
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(token);
	Locked<WalletImpl> walletImpl = m_sessionManager.Read()->GetWalletImpl(token);

	KeyChain keyChain = KeyChain::FromSeed(m_config, m_sessionManager.Read()->GetSeed(token));
	ed25519_keypair_t torKey = keyChain.DeriveED25519Key(path);

	std::shared_ptr<TorAddress> pTorAddress = pTorProcess->AddListener(torKey.secret_key, wallet.Read()->GetListenerPort());
	if (pTorAddress != nullptr)
	{
		wallet.Write()->SetTorAddress(*pTorAddress);
		walletImpl.Write()->SetTorAddress(*pTorAddress);
	}

	return wallet.Read()->GetTorAddress();
}

std::vector<GrinStr> WalletManager::GetAllAccounts() const
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

		auto pWallet = m_sessionManager.Read()->GetWallet(token).Read();
		return LoginResponse(
			token,
			pWallet->GetListenerPort(),
			pWallet->GetSlatepackAddress(),
			pWallet->GetTorAddress()
		);
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

void WalletManager::DeleteWallet(const GrinStr& username, const SecureString& password)
{
	WALLET_WARNING_F("Attempting to delete wallet with username: {}", username);

	try
	{
		GrinStr usernameLower = username.ToLower();
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
	const GrinStr& username,
	const SecureString& currentPassword,
	const SecureString& newPassword)
{
	GrinStr usernameLower = username.ToLower();
	EncryptedSeed encrypted = m_pWalletStore->LoadWalletSeed(usernameLower);
	SecureVector walletSeed = SeedEncrypter().DecryptWalletSeed(encrypted, currentPassword);

	// Valid password - Re-encrypt seed
	EncryptedSeed newSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, newPassword);
	m_pWalletStore->ChangePassword(usernameLower, newSeed);
}

Slate WalletManager::Send(const SendCriteria& sendCriteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(sendCriteria.GetToken());
	Locked<WalletImpl> wallet = m_sessionManager.Read()->GetWalletImpl(sendCriteria.GetToken());

	return SendSlateBuilder(m_config, m_pNodeClient).BuildSendSlate(
		wallet,
		masterSeed,
		sendCriteria.GetAmount().value_or(0),
		sendCriteria.GetFeeBase(),
		sendCriteria.GetNumOutputs(),
		!sendCriteria.GetAmount().has_value(), // TODO: Implement
		sendCriteria.GetAddress(),
		sendCriteria.GetSelectionStrategy(),
		sendCriteria.GetSlateVersion()
	);
}

Slate WalletManager::Receive(const ReceiveCriteria& receiveCriteria)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(receiveCriteria.GetToken());
	Locked<WalletImpl> wallet = m_sessionManager.Read()->GetWalletImpl(receiveCriteria.GetToken());

	return ReceiveSlateBuilder(m_config).AddReceiverData(
		wallet,
		masterSeed,
		receiveCriteria.GetSlate(),
		receiveCriteria.GetAddress()
	);
}

Slate WalletManager::Finalize(const FinalizeCriteria& finalizeCriteria, const TorProcess::Ptr& pTorProcess)
{
	const SecureVector masterSeed = m_sessionManager.Read()->GetSeed(finalizeCriteria.GetToken());
	Locked<Wallet> wallet = m_sessionManager.Read()->GetWallet(finalizeCriteria.GetToken());

	auto finalized = FinalizeSlateBuilder(wallet.Write().GetShared(), m_pNodeClient).Finalize(
		finalizeCriteria.GetSlate()
	);
	if (finalizeCriteria.GetPostMethod().has_value())
	{
		PostTransaction(
			finalized.second,
			finalizeCriteria.GetPostMethod().value(),
			pTorProcess
		);
	}

	return finalized.first;
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

bool WalletManager::RepostTx(const RepostTxCriteria& criteria, const TorProcess::Ptr& pTorProcess)
{
	Locked<Wallet> wallet = GetWallet(criteria.GetToken());

	auto pWalletTx = wallet.Read()->GetTransactionById(criteria.GetTxId());
	if (pWalletTx != nullptr) {
		if (pWalletTx->GetTransaction().has_value()) {
			PostMethodDTO postMethod(criteria.GetMethod(), criteria.GetGrinJoinAddress());
			return PostTransaction(pWalletTx->GetTransaction().value(), postMethod, pTorProcess);
		}
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
		std::shared_ptr<IWalletStore> pWalletDB = WalletDBAPI::OpenWalletDB(config);
		return std::shared_ptr<IWalletManager>(new WalletManager(config, pNodeClient, pWalletDB));
	}
}