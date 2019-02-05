#pragma once

#include "Wallet.h"

#include <Wallet/WalletServer.h>
#include <Wallet/WalletDB/WalletDB.h>

class WalletServer : public IWalletServer
{
public:
	WalletServer(const Config& config, const INodeClient& nodeClient, IWalletDB* pWalletDB);
	~WalletServer();

	virtual SecureString InitializeNewWallet(const std::string& username, const SecureString& password) override final;

	virtual bool Login(const std::string& username, const SecureString& password) override final;
	virtual void Logoff() override final;

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB* m_pWalletDB;

	Wallet* m_pWallet;
	SecureString* m_pPassword;
};