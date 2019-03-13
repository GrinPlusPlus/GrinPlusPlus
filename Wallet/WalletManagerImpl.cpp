#include "WalletManagerImpl.h"
#include "Keychain/SeedEncrypter.h"
#include "Keychain/Mnemonic.h"
#include "SlateBuilder.h"
#include "WalletRestorer.h"

#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/VectorUtil.h>

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
	const CBigInteger<32> walletSeed = RandomNumberGenerator::GenerateRandom32();
	const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, password);
	if (m_pWalletDB->CreateWallet(username, encryptedSeed))
	{
		SecureString walletWords = Mnemonic::CreateMnemonic(walletSeed.GetData(), std::make_optional(password));
		SessionToken token = m_sessionManager.Login(username, walletSeed);

		return std::make_optional<std::pair<SecureString, SessionToken>>(std::make_pair< SecureString, SessionToken>(std::move(walletWords), std::move(token)));
	}

	return std::nullopt;
}

std::optional<SessionToken> WalletManager::Restore(const std::string& username, const SecureString& password, const SecureString& walletWords)
{
	std::optional<std::vector<unsigned char>> entropyOpt = Mnemonic::ToEntropy(walletWords, std::make_optional<SecureString>(password));
	if (entropyOpt.has_value())
	{
		if (entropyOpt.value().size() == 32)
		{
			const CBigInteger<32> walletSeed(entropyOpt.value());
			const EncryptedSeed encryptedSeed = SeedEncrypter().EncryptWalletSeed(walletSeed, password);
			if (m_pWalletDB->CreateWallet(username, encryptedSeed))
			{
				return std::make_optional<SessionToken>(m_sessionManager.Login(username, walletSeed));
			}
		}
	}

	return std::nullopt;
}

bool WalletManager::CheckForOutputs(const SessionToken& token)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);
	const KeyChain keyChain = KeyChain::FromSeed(m_config, masterSeed);

	return WalletRestorer(m_config, m_nodeClient, keyChain).Restore(masterSeed, wallet);
}

std::unique_ptr<SessionToken> WalletManager::Login(const std::string& username, const SecureString& password)
{
	return m_sessionManager.Login(username, password);
}

void WalletManager::Logout(const SessionToken& token)
{
	m_sessionManager.Logout(token);
}

WalletSummary WalletManager::GetWalletSummary(const SessionToken& token)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);

	return wallet.GetWalletSummary(masterSeed);
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

// TODO: Return Slate with Transaction instead
std::unique_ptr<Transaction> WalletManager::Finalize(const SessionToken& token, const Slate& slate)
{
	const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	Wallet& wallet = m_sessionManager.GetWallet(token);

	return SlateBuilder(m_nodeClient).Finalize(wallet, masterSeed, slate);
}

bool WalletManager::PostTransaction(const SessionToken& token, const Transaction& transaction)
{
	//const CBigInteger<32> masterSeed = m_sessionManager.GetSeed(token);
	//Wallet& wallet = m_sessionManager.GetWallet(token);

	// TODO: Save transaction(TxLogEntry)

	return m_nodeClient.PostTransaction(transaction);
}

bool WalletManager::CancelByTxId(const SessionToken& token, const uint32_t walletTxId)
{
	// TODO: Implement
	return false;
}

bool WalletManager::CancelBySlateId(const SessionToken& token, const uuids::uuid& slateId)
{
	// TODO: Implement
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
		delete pWalletManager;
	}
}