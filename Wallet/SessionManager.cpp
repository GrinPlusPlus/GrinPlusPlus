#include "SessionManager.h"
#include "Keychain/SeedEncrypter.h"

#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Common/Util/VectorUtil.h>
#include <Infrastructure/Logger.h>

SessionManager::SessionManager(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB)
	: m_config(config), m_nodeClient(nodeClient), m_walletDB(walletDB)
{
	m_nextSessionId = RandomNumberGenerator::GenerateRandom(0, UINT64_MAX);
}

SessionManager::~SessionManager()
{
	for (auto iter = m_sessionsById.begin(); iter != m_sessionsById.end(); iter++)
	{
		delete iter->second;
	}
}

std::unique_ptr<SessionToken> SessionManager::Login(const std::string& username, const SecureString& password)
{
	LoggerAPI::LogDebug("SessionManager::Login - Logging in with username: " + username);
	std::unique_ptr<EncryptedSeed> pSeed = m_walletDB.LoadWalletSeed(username);
	if (pSeed != nullptr)
	{
		std::optional<SecureVector> decryptedSeedOpt = SeedEncrypter().DecryptWalletSeed(*pSeed, password);
		if (decryptedSeedOpt.has_value())
		{
			LoggerAPI::LogInfo("SessionManager::Login - Valid password provided. Logging in now.");

			m_walletDB.OpenWallet(username);
			return std::make_unique<SessionToken>(Login(username, decryptedSeedOpt.value()));
		}

		LoggerAPI::LogError("SessionManager::Login - Wallet seed not decrypted. Wrong password?");
	}

	LoggerAPI::LogError("SessionManager::Login - Wallet seed not found.");
	return std::unique_ptr<SessionToken>(nullptr);
}

SessionToken SessionManager::Login(const std::string& username, const SecureVector& seed)
{
	KeyChain keyChain = KeyChain::FromSeed(m_config, seed);
	LoggerAPI::LogTrace("SessionManager::Login - KeyChain loaded successfully from seed.");

	Wallet* pWallet = Wallet::LoadWallet(m_config, m_nodeClient, m_walletDB, username);
	LoggerAPI::LogTrace("SessionManager::Login - Wallet loaded successfully.");

	const CBigInteger<32> hash = Crypto::SHA256((const std::vector<unsigned char>&)seed);
	const std::vector<unsigned char> checksum(hash.GetData().cbegin(), hash.GetData().cbegin() + 4);
	SecureVector seedWithChecksum = SecureVector(seed.begin(), seed.end());
	seedWithChecksum.insert(seedWithChecksum.end(), checksum.begin(), checksum.end());
	LoggerAPI::LogTrace("SessionManager::Login - seedWithChecksum calculated.");

	SecureVector tokenKey = RandomNumberGenerator::GenerateRandomBytes(seedWithChecksum.size());

	std::vector<unsigned char> encryptedSeedWithCS(seedWithChecksum.size());
	for (size_t i = 0; i < seedWithChecksum.size(); i++)
	{
		encryptedSeedWithCS[i] = seedWithChecksum[i] ^ tokenKey[i];
	}

	KeyChain grinboxKeyChain = KeyChain::ForGrinbox(m_config, seed);
	LoggerAPI::LogTrace("SessionManager::Login - Grinbox keychain loaded successfully.");

	std::unique_ptr<SecretKey> pGrinboxAddress = grinboxKeyChain.DerivePrivateKey(KeyChainPath(std::vector<uint32_t>({ 0 }))); // TODO: Determine KeyChainPath
	std::vector<unsigned char> encryptedGrinboxAddress(32);
	for (size_t i = 0; i < 32; i++)
	{
		encryptedGrinboxAddress[i] = pGrinboxAddress->GetBytes()[i] ^ hash[i];
	}

	const uint64_t sessionId = m_nextSessionId++;
	m_sessionsById[sessionId] = new LoggedInSession(pWallet, std::move(encryptedSeedWithCS), std::move(encryptedGrinboxAddress));
	LoggerAPI::LogDebug("SessionManager::Login - Session created successfully.");

	return SessionToken(sessionId, std::vector<unsigned char>(tokenKey.begin(), tokenKey.end()));
}

void SessionManager::Logout(const SessionToken& token)
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		delete iter->second;
		m_sessionsById.erase(iter);
	}
}

SecureVector SessionManager::GetSeed(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		const LoggedInSession* pSession = iter->second;
		
		std::vector<unsigned char> seedWithCS(pSession->m_encryptedSeedWithCS.size());
		for (size_t i = 0; i < seedWithCS.size(); i++)
		{
			seedWithCS[i] = pSession->m_encryptedSeedWithCS[i] ^ token.GetTokenKey()[i];
		}

		SecureVector seed(seedWithCS.cbegin(), seedWithCS.cbegin() + seedWithCS.size() - 4);
		CBigInteger<32> hash = Crypto::SHA256((const std::vector<unsigned char>&)seed);
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

SecretKey SessionManager::GetGrinboxAddress(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		const SecureVector seed = GetSeed(token);
		const SecretKey hash = Crypto::SHA256((const std::vector<unsigned char>&)seed);

		const LoggedInSession* pSession = iter->second;
		SecureVector grinboxAddress(32);
		for (size_t i = 0; i < 32; i++)
		{
			grinboxAddress[i] = pSession->m_encryptedGrinboxAddress[i] ^ hash.GetBytes()[i];
		}

		return SecretKey(CBigInteger<32>((const std::vector<unsigned char>&)grinboxAddress));
	}

	throw SessionTokenException();
}

LockedWallet SessionManager::GetWallet(const SessionToken& token)
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		LoggedInSession* pSession = iter->second;

		return LockedWallet(pSession->m_mutex, *(pSession->m_pWallet));
	}

	throw SessionTokenException();
}

//const Wallet& SessionManager::GetWallet(const SessionToken& token) const
//{
//	auto iter = m_sessionsById.find(token.GetSessionId());
//	if (iter != m_sessionsById.end())
//	{
//		const LoggedInSession* pSession = iter->second;
//
//		return *(pSession->m_pWallet);
//	}
//
//	throw SessionTokenException();
//}