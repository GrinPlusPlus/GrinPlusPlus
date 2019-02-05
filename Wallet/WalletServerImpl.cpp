#include "WalletServerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"

#include <Crypto/RandomNumberGenerator.h>

WalletServer::WalletServer(const Config& config, const INodeClient& nodeClient, IWalletDB* pWalletDB)
	: m_config(config), m_nodeClient(nodeClient), m_pWalletDB(pWalletDB)
{

}

WalletServer::~WalletServer()
{
	Logoff();
	WalletDBAPI::CloseWalletDB(m_pWalletDB);
}

SecureString WalletServer::InitializeNewWallet(const std::string& username, const SecureString& password)
{
	const CBigInteger<32> walletSeed = RandomNumberGenerator::GenerateRandom32();
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, password);
	if (m_pWalletDB->CreateWallet(username, encryptedSeed))
	{
		return Mnemonic::CreateMnemonic(walletSeed.GetData(), std::make_optional(password));
	}

	return SecureString("");
}

bool WalletServer::Login(const std::string& username, const SecureString& password)
{
	std::unique_ptr<EncryptedSeed> pSeed = m_pWalletDB->LoadWalletSeed(username);
	if (pSeed != nullptr)
	{
		CBigInteger<32> decryptedSeed = SeedEncrypter().DecryptWalletSeed(*pSeed, password);
		if (decryptedSeed != CBigInteger<32>::ValueOf(0))
		{
			Logoff();

			m_pWallet = Wallet::LoadWallet(m_config, m_nodeClient, m_walletDB, username, *pSeed);
			m_pPassword = new SecureString(password);

			return true;
		}
	}

	return false;
}

void WalletServer::Logoff()
{
	delete m_pWallet;
	m_pWallet = nullptr;

	delete m_pPassword;
	m_pPassword = nullptr;
}

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletServer* StartWalletServer(const Config& config, const INodeClient& nodeClient)
	{
		IWalletDB* pWalletDB = WalletDBAPI::OpenWalletDB(config);
		return new WalletServer(config, nodeClient, pWalletDB)
	}

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletServer(IWalletServer* pWalletServer)
	{
		delete pWalletServer;
	}
}