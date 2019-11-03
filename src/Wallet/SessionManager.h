#pragma once

#include "Wallet.h"
#include "ForeignController.h"
#include "LoggedInSession.h"

#include <Crypto/SecretKey.h>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/SessionToken.h>
#include <memory>
#include <unordered_map>

// Forward Declarations
class IWalletManager;

class SessionManager
{
public:
	SessionManager(const Config& config, INodeClientConstPtr pNodeClient, IWalletDBPtr pWalletDB, IWalletManager& walletManager);
	~SessionManager();

	SessionToken Login(const std::string& username, const SecureString& password);
	SessionToken Login(const std::string& username, const SecureVector& seed);
	void Logout(const SessionToken& token);

	SecureVector GetSeed(const SessionToken& token) const;
	SecretKey GetGrinboxAddress(const SessionToken& token) const;
	Locked<Wallet> GetWallet(const SessionToken& token);

private:
	std::unordered_map<uint64_t, LoggedInSession*> m_sessionsById;
	// TODO: Keep multimap of sessions per username

	uint64_t m_nextSessionId;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	IWalletDBPtr m_pWalletDB;
	ForeignController m_foreignController;
};