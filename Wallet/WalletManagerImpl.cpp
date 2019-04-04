#include "WalletManagerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder/ReceiveSlateBuilder.h"
#include "SlateBuilder/SendSlateBuilder.h"
#include "SlateBuilder/FinalizeSlateBuilder.h"
#include "WalletRestorer.h"
//#include "Grinbox/GrinboxConnection.h"

#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/StringUtil.h>
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
	const SecretKey walletSeed = RandomNumberGenerator::GenerateRandom32();
	const SecureVector walletSeedBytes(walletSeed.GetBytes().GetData().begin(), walletSeed.GetBytes().GetData().end());
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeedBytes, password);

	const std::string usernameLower = StringUtil::ToLower(username);
	if (m_pWalletDB->CreateWallet(usernameLower, encryptedSeed))
	{
		SecureString walletWords = Mnemonic::CreateMnemonic(walletSeed.GetBytes().GetData(), std::make_optional(password));
		SessionToken token = m_sessionManager.Login(usernameLower, walletSeedBytes);

		return std::make_optional<std::pair<SecureString, SessionToken>>(std::make_pair<SecureString, SessionToken>(std::move(walletWords), std::move(token)));
	}

	return std::nullopt;
}

std::optional<SessionToken> WalletManager::Restore(const std::string& username, const SecureString& password, const SecureString& walletWords)
{
	std::optional<SecureVector> entropyOpt = Mnemonic::ToEntropy(walletWords, std::make_optional<SecureString>(password));
	if (entropyOpt.has_value())
	{
		const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(entropyOpt.value(), password);
		const std::string usernameLower = StringUtil::ToLower(username);
		if (m_pWalletDB->CreateWallet(usernameLower, encryptedSeed))
		{
			return std::make_optional<SessionToken>(m_sessionManager.Login(usernameLower, entropyOpt.value()));
		}
	}

	return std::nullopt;
}

bool WalletManager::CheckForOutputs(const SessionToken& token, const bool fromGenesis)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);
	const KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);

	return WalletRestorer(m_config, m_nodeClient, keyChain).Restore(masterSeed, wallet.GetWallet(), fromGenesis);
}

std::vector<std::string> WalletManager::GetAllAccounts() const
{
	return m_pWalletDB->GetAccounts();
}

std::unique_ptr<SessionToken> WalletManager::Login(const std::string& username, const SecureString& password)
{
	const std::string usernameLower = StringUtil::ToLower(username);
	std::unique_ptr<SessionToken> pToken = m_sessionManager.Login(usernameLower, password);
	if (pToken != nullptr)
	{
		//auto asyncCheckForOutputs =
		//	[this](SessionToken token) -> void { this->CheckForOutputs(token); };

		//std::thread thread(asyncCheckForOutputs, *pToken);
		//thread.detach();
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

std::vector<WalletTx> WalletManager::GetTransactions(const SessionToken& token)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	return wallet.GetWallet().GetTransactions(masterSeed);
}

std::unique_ptr<Slate> WalletManager::Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs)
{
	const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	LockedWallet wallet = m_sessionManager.GetWallet(token);

	return SendSlateBuilder(m_nodeClient).BuildSendSlate(wallet.GetWallet(), masterSeed, amount, feeBase, numChangeOutputs, messageOpt, strategy);
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
	//const SecureVector masterSeed = m_sessionManager.GetSeed(token);
	//LockedWallet wallet = m_sessionManager.GetWallet(token);

	// TODO: Save transaction(TxLogEntry)

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
	//	GrinboxConnection* pConnection = new GrinboxConnection();
		//pConnection->Connect();
		return new WalletManager(config, nodeClient, pWalletDB);
	}

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletManager(IWalletManager* pWalletManager)
	{
		delete pWalletManager;
	}
}