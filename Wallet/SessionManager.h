#pragma once

#include "Wallet.h"
#include "LockedWallet.h"

#include <Crypto/SecretKey.h>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SessionToken.h>
#include <memory>
#include <map>

class SessionManager
{
public:
	SessionManager(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB);
	~SessionManager();

	std::unique_ptr<SessionToken> Login(const std::string& username, const SecureString& password);
	SessionToken Login(const std::string& username, const SecureVector& seed);
	void Logout(const SessionToken& token);

	SecureVector GetSeed(const SessionToken& token) const;
	SecretKey GetGrinboxAddress(const SessionToken& token) const;
	LockedWallet GetWallet(const SessionToken& token);
	//const Wallet& GetWallet(const SessionToken& token) const;

private:
	struct LoggedInSession
	{
		LoggedInSession(Wallet* pWallet, std::vector<unsigned char>&& encryptedSeedWithCS, std::vector<unsigned char>&& encryptedGrinboxAddress)
			: m_pWallet(pWallet), m_encryptedSeedWithCS(std::move(encryptedSeedWithCS)), m_encryptedGrinboxAddress(std::move(encryptedGrinboxAddress))
		{

		}

		~LoggedInSession()
		{
			delete m_pWallet;
		}

		std::mutex m_mutex;
		Wallet* m_pWallet;
		std::vector<unsigned char> m_encryptedSeedWithCS;
		std::vector<unsigned char> m_encryptedGrinboxAddress;
	};

	std::map<uint64_t, LoggedInSession*> m_sessionsById;
	uint64_t m_nextSessionId;

	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
};