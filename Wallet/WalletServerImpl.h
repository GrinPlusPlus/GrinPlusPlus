#pragma once

#include "SessionManager.h"

#include <Wallet/WalletServer.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <map>

class WalletServer : public IWalletServer
{
public:
	WalletServer(const Config& config, const INodeClient& nodeClient, IWalletDB* pWalletDB);
	~WalletServer();

	virtual SecureString InitializeNewWallet(const std::string& username, const SecureString& password) override final;

	virtual std::unique_ptr<SessionToken> Login(const std::string& username, const SecureString& password) override final;
	virtual void Logoff(const SessionToken& token) override final;

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB* m_pWalletDB;
	SessionManager m_sessionManager;
};