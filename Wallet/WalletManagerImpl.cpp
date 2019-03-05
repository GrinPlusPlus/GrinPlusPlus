#include "WalletManagerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder.h"

#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>

WalletManager::WalletManager(const Config& config, const INodeClient& nodeClient, IWalletDB* pWalletDB)
	: m_config(config), m_nodeClient(nodeClient), m_pWalletDB(pWalletDB), m_sessionManager(config, nodeClient, *pWalletDB)
{

}

WalletManager::~WalletManager()
{
	WalletDBAPI::CloseWalletDB(m_pWalletDB);
}

SecureString WalletManager::InitializeNewWallet(const std::string& username, const SecureString& password)
{
	const CBigInteger<32> walletSeed = RandomNumberGenerator::GenerateRandom32();
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, password);
	if (m_pWalletDB->CreateWallet(username, encryptedSeed))
	{
		return Mnemonic::CreateMnemonic(walletSeed.GetData(), std::make_optional(password));
	}

	return SecureString("");
}

std::unique_ptr<SessionToken> WalletManager::Login(const std::string& username, const SecureString& password)
{
	return m_sessionManager.Login(username, password);
}

void WalletManager::Logout(const SessionToken& token)
{
	m_sessionManager.Logout(token);
}

std::unique_ptr<Slate> WalletManager::Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);

	return SlateBuilder(m_nodeClient).BuildSendSlate(wallet, masterSeed, amount, feeBase, messageOpt, strategy);
}

bool WalletManager::Receive(const SessionToken& token, Slate& slate, const std::optional<std::string>& messageOpt)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);

	return SlateBuilder(m_nodeClient).AddReceiverData(wallet, masterSeed, slate, messageOpt);
}

std::unique_ptr<Transaction> WalletManager::Finalize(const SessionToken& token, const Slate& slate)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);

	return SlateBuilder(m_nodeClient).Finalize(wallet, masterSeed, slate);
}

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletManager* StartWalletManager(const Config& config, const INodeClient& nodeClient)
	{
		IWalletDB* pWalletDB = WalletDBAPI::OpenWalletDB(config);
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