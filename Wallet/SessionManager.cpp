#include "SessionManager.h"
#include "Keychain/SeedEncrypter.h"

#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Wallet/SessionTokenException.h>
#include <Common/Util/VectorUtil.h>

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
	std::unique_ptr<EncryptedSeed> pSeed = m_walletDB.LoadWalletSeed(username);
	if (pSeed != nullptr)
	{
		std::optional<CBigInteger<32>> decryptedSeedOpt = SeedEncrypter().DecryptWalletSeed(*pSeed, password);
		if (decryptedSeedOpt.has_value())
		{
			return std::make_unique<SessionToken>(Login(username, decryptedSeedOpt.value()));
		}
	}

	return std::unique_ptr<SessionToken>(nullptr);
}

SessionToken SessionManager::Login(const std::string& username, const CBigInteger<32>& seed)
{
	KeyChain keyChain = KeyChain::FromSeed(m_config, seed);
	Wallet* pWallet = Wallet::LoadWallet(m_config, m_nodeClient, m_walletDB, username);

	const CBigInteger<32> hash = Crypto::SHA256(seed.GetData());
	const std::vector<unsigned char> checksum(hash.GetData().cbegin(), hash.GetData().cbegin() + 4);
	const std::vector<unsigned char> seedWithChecksum = VectorUtil::Concat(seed.GetData(), checksum);

	std::vector<unsigned char> tokenKey = RandomNumberGenerator::GenerateRandomBytes(seedWithChecksum.size());
	std::vector<unsigned char> encryptedSeedWithCS(36);
	for (size_t i = 0; i < 36; i++)
	{
		encryptedSeedWithCS[i] = seedWithChecksum[i] ^ tokenKey[i];
	}

	const uint64_t sessionId = m_nextSessionId++;
	m_sessionsById[sessionId] = new LoggedInSession(pWallet, std::move(encryptedSeedWithCS));

	return SessionToken(sessionId, std::move(tokenKey));
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

CBigInteger<32> SessionManager::GetSeed(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		const LoggedInSession* pSession = iter->second;
		
		std::vector<unsigned char> seedWithCS(36);
		for (size_t i = 0; i < 36; i++)
		{
			seedWithCS[i] = pSession->m_encryptedSeedWithCS[i] ^ token.GetTokenKey()[i];
		}

		CBigInteger<32> seed(std::vector<unsigned char>(seedWithCS.cbegin(), seedWithCS.cbegin() + 32));
		CBigInteger<32> hash = Crypto::SHA256(seed.GetData());
		for (int i = 0; i < 4; i++)
		{
			if (seedWithCS[32 + i] != hash[i])
			{
				throw SessionTokenException();
			}
		}

		return seed;
	}

	throw SessionTokenException();
}

Wallet& SessionManager::GetWallet(const SessionToken& token)
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		const LoggedInSession* pSession = iter->second;

		return *(pSession->m_pWallet);
	}

	throw SessionTokenException();
}

const Wallet& SessionManager::GetWallet(const SessionToken& token) const
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		const LoggedInSession* pSession = iter->second;

		return *(pSession->m_pWallet);
	}

	throw SessionTokenException();
}