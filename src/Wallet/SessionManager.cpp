#include "SessionManager.h"
#include "Keychain/SeedEncrypter.h"

#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Common/Util/VectorUtil.h>
#include <Infrastructure/Logger.h>

SessionManager::SessionManager(const Config& config, INodeClientConstPtr pNodeClient, IWalletDBPtr pWalletDB, std::shared_ptr<ForeignController> pForeignController)
	: m_config(config), m_pNodeClient(pNodeClient), m_pWalletDB(pWalletDB), m_pForeignController(pForeignController)
{
	m_nextSessionId = RandomNumberGenerator::GenerateRandom(0, UINT64_MAX);
}

SessionManager::~SessionManager()
{
	for (auto iter = m_sessionsById.begin(); iter != m_sessionsById.end(); iter++)
	{
		m_pForeignController->StopListener(iter->second->m_wallet.Read()->GetUsername());
	}
}

Locked<SessionManager> SessionManager::Create(
	const Config& config,
	INodeClientConstPtr pNodeClient,
	IWalletDBPtr pWalletDB,
	IWalletManager& walletManager)
{
	std::shared_ptr<ForeignController> pForeignController = std::shared_ptr<ForeignController>(new ForeignController(config, walletManager));
	return Locked<SessionManager>(std::make_shared<SessionManager>(SessionManager(config, pNodeClient, pWalletDB, pForeignController)));
}

SessionToken SessionManager::Login(const std::string& username, const SecureString& password)
{
	WALLET_DEBUG_F("Logging in with username: %s", username);
	std::unique_ptr<EncryptedSeed> pSeed = m_pWalletDB->LoadWalletSeed(username);
	if (pSeed != nullptr)
	{
		std::optional<SecureVector> decryptedSeedOpt = SeedEncrypter().DecryptWalletSeed(*pSeed, password);
		if (decryptedSeedOpt.has_value())
		{
			WALLET_INFO("Valid password provided. Logging in now.");

			m_pWalletDB->OpenWallet(username, decryptedSeedOpt.value());
			return Login(username, decryptedSeedOpt.value());
		}

		WALLET_ERROR("Wallet seed not decrypted. Wrong password?");
	}
	else
	{
		WALLET_ERROR("Wallet seed not found.");
	}

	throw SessionTokenException();
}

SessionToken SessionManager::Login(const std::string& username, const SecureVector& seed)
{
	const CBigInteger<32> hash = Crypto::SHA256((const std::vector<unsigned char>&)seed);
	const std::vector<unsigned char> checksum(hash.GetData().cbegin(), hash.GetData().cbegin() + 4);
	SecureVector seedWithChecksum = SecureVector(seed.begin(), seed.end());
	seedWithChecksum.insert(seedWithChecksum.end(), checksum.begin(), checksum.end());

	SecureVector tokenKey = RandomNumberGenerator::GenerateRandomBytes(seedWithChecksum.size());

	SecureVector encryptedSeedWithCS(seedWithChecksum.size());
	for (size_t i = 0; i < seedWithChecksum.size(); i++)
	{
		encryptedSeedWithCS[i] = seedWithChecksum[i] ^ tokenKey[i];
	}

	KeyChain grinboxKeyChain = KeyChain::ForGrinbox(m_config, seed);

	std::unique_ptr<SecretKey> pGrinboxAddress = grinboxKeyChain.DerivePrivateKey(KeyChainPath(std::vector<uint32_t>({ 0 }))); // TODO: Determine KeyChainPath
	SecureVector encryptedGrinboxAddress(32);
	for (size_t i = 0; i < 32; i++)
	{
		encryptedGrinboxAddress[i] = pGrinboxAddress->GetBytes()[i] ^ hash[i];
	}

	const uint64_t sessionId = m_nextSessionId++;
	SessionToken token(sessionId, std::vector<unsigned char>(tokenKey.begin(), tokenKey.end()));

	Locked<Wallet> wallet = Wallet::LoadWallet(m_config, m_pNodeClient, m_pWalletDB, username);

	LoggedInSession* pSession = new LoggedInSession(wallet, std::move(encryptedSeedWithCS), std::move(encryptedGrinboxAddress));
	m_sessionsById[sessionId] = std::shared_ptr<LoggedInSession>(pSession);

	std::optional<TorAddress> torAddressOpt = m_pForeignController->StartListener(username, token, seed);
	if (torAddressOpt.has_value())
	{
		wallet.Write()->SetTorAddress(torAddressOpt.value());
	}

	return token;
}

void SessionManager::Logout(const SessionToken& token)
{
	auto iter = m_sessionsById.find(token.GetSessionId());
	if (iter != m_sessionsById.end())
	{
		m_pForeignController->StopListener(iter->second->m_wallet.Read()->GetUsername());
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

		std::shared_ptr<const LoggedInSession> pSession = iter->second;
		SecureVector grinboxAddress(32);
		for (size_t i = 0; i < 32; i++)
		{
			grinboxAddress[i] = pSession->m_encryptedGrinboxAddress[i] ^ hash.GetBytes()[i];
		}

		return SecretKey(CBigInteger<32>((const std::vector<unsigned char>&)grinboxAddress));
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