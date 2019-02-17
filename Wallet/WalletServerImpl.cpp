#include "WalletServerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder.h"

#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>

WalletServer::WalletServer(const Config& config, const INodeClient& nodeClient, IWalletDB* pWalletDB)
	: m_config(config), m_nodeClient(nodeClient), m_pWalletDB(pWalletDB), m_sessionManager(config, nodeClient, *pWalletDB)
{

}

WalletServer::~WalletServer()
{
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

std::unique_ptr<SessionToken> WalletServer::Login(const std::string& username, const SecureString& password)
{
	return m_sessionManager.Login(username, password);
}

void WalletServer::Logoff(const SessionToken& token)
{
	m_sessionManager.Logoff(token);
}

std::unique_ptr<Slate> WalletServer::Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::string& message, const ESelectionStrategy& strategy, const ESendMethod& method, const std::string& destination)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);

	std::unique_ptr<Slate> pSlate = SlateBuilder(m_nodeClient).BuildSendSlate(wallet, masterSeed, amount, feeBase, message, strategy);
	if (pSlate != nullptr)
	{
		// TODO: Send slate to destination using ESendMethod.
		return pSlate;
	}

	return std::unique_ptr<Slate>(nullptr);
}

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletServer* StartWalletServer(const Config& config, const INodeClient& nodeClient)
	{
		IWalletDB* pWalletDB = WalletDBAPI::OpenWalletDB(config);
		return new WalletServer(config, nodeClient, pWalletDB);
	}

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletServer(IWalletServer* pWalletServer)
	{
		delete pWalletServer;
	}
}