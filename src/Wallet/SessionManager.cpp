#include "SessionManager.h"
#include "ForeignController.h"
#include "Keychain/SeedEncrypter.h"

#include <Common/Logger.h>
#include <Crypto/Hasher.h>
#include <Crypto/CSPRNG.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Wallet/WalletDB/WalletStore.h>

SessionManager::SessionManager(
	const Config& config,
	const std::shared_ptr<const INodeClient>& pNodeClient,
	const std::shared_ptr<IWalletStore>& pWalletDB,
	std::unique_ptr<ForeignController>&& pForeignController)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletDB(pWalletDB),
	m_pForeignController(std::move(pForeignController))
{
	m_nextSessionId = CSPRNG::GenerateRandom(0, UINT64_MAX);
}

SessionManager::~SessionManager()
{
	LOG_INFO("Shutting down session manager");
	for (auto iter = m_sessionsById.begin(); iter != m_sessionsById.end(); iter++)
	{
		m_pForeignController->StopListener(iter->second->m_wallet.Read()->GetUsername());
	}
}

Locked<SessionManager> SessionManager::Create(
	const Config& config,
	const std::shared_ptr<const INodeClient>& pNodeClient,
	const std::shared_ptr<IWalletStore>& pWalletDB,
	IWalletManager& walletManager)
{
	auto pForeignController = std::make_unique<ForeignController>(walletManager);
	auto pSessionManager = std::make_shared<SessionManager>(
		config,
		pNodeClient,
		pWalletDB,
		std::move(pForeignController)
	);
	return Locked<SessionManager>(pSessionManager);
}

void SessionManager::Authenticate(const std::string& username, const SecureString& password) const
{
	std::unique_ptr<EncryptedSeed> pSeed = nullptr;

	try
	{
		pSeed = std::make_unique<EncryptedSeed>(m_pWalletDB->LoadWalletSeed(username));
	}
	catch (std::exception&)
	{
		WALLET_ERROR_F("User '{}' not found.", username);
		throw;
	}

	try
	{
		SecureVector decryptedSeed = SeedEncrypter().DecryptWalletSeed(*pSeed, password);
	}
	catch (std::exception&)
	{
		WALLET_ERROR_F("Invalid password provided for user '{}'", username);
		throw;
	}
}

SecureVector SessionManager::GetSeed(const std::string& username, const SecureString& password) const
{
	std::unique_ptr<EncryptedSeed> pSeed = nullptr;

	try
	{
		pSeed = std::make_unique<EncryptedSeed>(m_pWalletDB->LoadWalletSeed(username));
	}
	catch (std::exception&)
	{
		WALLET_ERROR_F("User '{}' not found.", username);
		throw;
	}

	try
	{
		return SeedEncrypter().DecryptWalletSeed(*pSeed, password);
	}
	catch (std::exception&)
	{
		WALLET_ERROR_F("Invalid password provided for user '{}'", username);
		throw;
	}
}

SessionToken SessionManager::Login(
	const TorProcess::Ptr& pTorProcess,
	const std::string& username,
	const SecureString& password)
{
	WALLET_DEBUG_F("Logging in with username: {}", username);
	try
	{
		EncryptedSeed seed = m_pWalletDB->LoadWalletSeed(username);

		SecureVector decryptedSeed = SeedEncrypter().DecryptWalletSeed(seed, password);

		WALLET_INFO("Valid password provided. Logging in now.");
		WALLET_INFO("Calling m_pWalletDB->OpenWallet");
		m_pWalletDB->OpenWallet(username, decryptedSeed);
		WALLET_INFO("Returning Login");
		return Login(pTorProcess, username, decryptedSeed);
	}
	catch (std::exception&)
	{
		WALLET_ERROR("Wallet seed not decrypted. Wrong password?");
		throw;
	}
}

SessionToken SessionManager::Login(
	const TorProcess::Ptr& pTorProcess,
	const std::string& username,
	const SecureVector& seed)
{
	WALLET_INFO("Getting seed...");
	Hash hash = Hasher::SHA256((const std::vector<unsigned char>&)seed);
	std::vector<unsigned char> checksum(
		hash.GetData().cbegin(),
		hash.GetData().cbegin() + 4
	);
	SecureVector seedWithChecksum = seed;
	seedWithChecksum.insert(seedWithChecksum.end(), checksum.begin(), checksum.end());

	SecureVector tokenKey = CSPRNG::GenerateRandomBytes(seedWithChecksum.size());

	SecureVector encryptedSeedWithCS(seedWithChecksum.size());
	for (size_t i = 0; i < seedWithChecksum.size(); i++)
	{
		encryptedSeedWithCS[i] = seedWithChecksum[i] ^ tokenKey[i];
	}

	const uint64_t sessionId = m_nextSessionId++;
	SessionToken token(sessionId, tokenKey);

	WALLET_INFO("Opening wallet...");
	auto pWalletDB = m_pWalletDB->OpenWallet(username, seed);
	WALLET_INFO("Wallet opened.");

	WALLET_INFO("Loading wallet...");
	Locked<WalletImpl> walletImpl = WalletImpl::LoadWallet(
		m_config,
		seed,
		m_pNodeClient,
		pWalletDB,
		username
	);
	WALLET_INFO("Wallet loaded.");

	Locked<Wallet> wallet = std::make_shared<Wallet>(
		pWalletDB,
		token,
		seed,
		username,
		KeyChainPath::FromString("m/0/0"),
		walletImpl.Read()->GetSlatepackAddress()
	);

	m_sessionsById[sessionId] = std::make_shared<LoggedInSession>(
		wallet,
		walletImpl,
		std::move(encryptedSeedWithCS)
	);
	WALLET_INFO("KeyChain from seed...");
	KeyChain keyChain = KeyChain::FromSeed(m_config, seed);
	WALLET_INFO("Getting listener...");
	auto listenerInfo = m_pForeignController->StartListener(
		pTorProcess,
		username,
		token,
		keyChain
	);
	WALLET_INFO("Setting listener port...");
	wallet.Write()->SetListenerPort(listenerInfo.first);
	walletImpl.Write()->SetListenerPort(listenerInfo.first);
	WALLET_INFO("Listener port set.");
	if (listenerInfo.second.has_value()) {
		WALLET_INFO("Setting Tor Address...");
		wallet.Write()->SetTorAddress(listenerInfo.second.value());
		walletImpl.Write()->SetTorAddress(listenerInfo.second.value());
		WALLET_INFO("Tor Address set.");
	}

	WALLET_INFO("Returning TOKEN.");
	return token;
}

void SessionManager::Logout(const SessionToken& token)
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		std::string username = iter->second->m_wallet.Read()->GetUsername();
		m_pForeignController->StopListener(iter->second->m_wallet.Read()->GetUsername());
		if (!m_pForeignController->IsListening(username)) {
			m_pWalletDB->CloseWallet(username);
		}
		m_sessionsById.erase(iter);
	}
}

SecureVector SessionManager::GetSeed(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		std::shared_ptr<const LoggedInSession> pSession = iter->second;
		
		std::vector<unsigned char> seedWithCS(pSession->m_encryptedSeedWithCS.size());
		for (size_t i = 0; i < seedWithCS.size(); i++)
		{
			seedWithCS[i] = pSession->m_encryptedSeedWithCS[i] ^ token.GetTokenKey()[i];
		}

		SecureVector seed(seedWithCS.cbegin(), seedWithCS.cbegin() + seedWithCS.size() - 4);
		CBigInteger<32> hash = Hasher::SHA256((const std::vector<unsigned char>&)seed);
		for (int i = 0; i < 4; i++)
		{
			if (seedWithCS[(seedWithCS.size() - 4) + i] != hash[i])
			{
				throw SessionTokenException();
			}
		}

		return seed;
	}

	throw SessionTokenException();
}

Locked<Wallet> SessionManager::GetWallet(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		return iter->second->m_wallet;
	}

	throw SessionTokenException();
}

Locked<WalletImpl> SessionManager::GetWalletImpl(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		return iter->second->m_walletImpl;
	}

	throw SessionTokenException();
}