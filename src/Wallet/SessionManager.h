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
	SessionManager(
		const Config& config,
		const INodeClientConstPtr& pNodeClient,
		const std::shared_ptr<IWalletStore>& pWalletDB,
		const std::shared_ptr<ForeignController>& pForeignController
	);
	~SessionManager();

	static Locked<SessionManager> Create(
		const Config& config,
		const INodeClientConstPtr& pNodeClient,
		const std::shared_ptr<IWalletStore>& pWalletDB,
		IWalletManager& walletManager
	);

	void Authenticate(const std::string& username, const SecureString& password) const;
	SessionToken Login(
		const TorProcess::Ptr& pTorProcess,
		const std::string& username,
		const SecureString& password
	);
	SessionToken Login(
		const TorProcess::Ptr& pTorProcess,
		const std::string& username,
		const SecureVector& seed
	);
	void Logout(const SessionToken& token);

	SecureVector GetSeed(const SessionToken& token) const;
	SecureVector GetSeed(const std::string& username, const SecureString& password) const;
	Locked<Wallet> GetWallet(const SessionToken& token) const;

private:
	std::unordered_map<uint64_t, std::shared_ptr<LoggedInSession>> m_sessionsById;
	// TODO: Keep multimap of sessions per username

	uint64_t m_nextSessionId;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	std::shared_ptr<IWalletStore> m_pWalletDB;
	std::shared_ptr<ForeignController> m_pForeignController;
};