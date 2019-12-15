#pragma once

#include "Wallet.h"
#include "ForeignController.h"
#include "LoggedInSession.h"

#include <Crypto/SecretKey.h>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletStore.h>
#include <Wallet/SessionToken.h>
#include <memory>
#include <unordered_map>

// Forward Declarations
class IWalletManager;

class SessionManager
{
public:
	~SessionManager();

	static Locked<SessionManager> Create(
		const Config& config,
		INodeClientConstPtr pNodeClient,
		std::shared_ptr<IWalletStore> pWalletDB,
		IWalletManager& walletManager
	);

	SessionToken Login(const std::string& username, const SecureString& password);
	SessionToken Login(const std::string& username, const SecureVector& seed);
	void Logout(const SessionToken& token);

	SecureVector GetSeed(const SessionToken& token) const;
	Locked<Wallet> GetWallet(const SessionToken& token) const;

private:
	SessionManager(
		const Config& config,
		INodeClientConstPtr pNodeClient,
		std::shared_ptr<IWalletStore> pWalletDB,
		std::shared_ptr<ForeignController> pForeignController
	);

	std::unordered_map<uint64_t, std::shared_ptr<LoggedInSession>> m_sessionsById;
	// TODO: Keep multimap of sessions per username

	uint64_t m_nextSessionId;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	std::shared_ptr<IWalletStore> m_pWalletDB;
	std::shared_ptr<ForeignController> m_pForeignController;
};