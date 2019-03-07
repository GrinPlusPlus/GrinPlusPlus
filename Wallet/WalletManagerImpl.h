#pragma once

#include "SessionManager.h"

#include <Wallet/WalletManager.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <map>

class WalletManager : public IWalletManager
{
public:
	WalletManager(const Config& config, const INodeClient& nodeClient, IWalletDB* pWalletDB);
	~WalletManager();

	virtual std::optional<std::pair<SecureString, SessionToken>> InitializeNewWallet(const std::string& username, const SecureString& password) override final;

	virtual std::unique_ptr<SessionToken> Login(const std::string& username, const SecureString& password) override final;
	virtual void Logout(const SessionToken& token) override final;

	virtual WalletSummary GetWalletSummary(const SessionToken& token, const uint64_t minimumConfirmations) const override final;

	virtual std::unique_ptr<Slate> Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy) override final;
	virtual bool Receive(const SessionToken& token, Slate& slate, const std::optional<std::string>& messageOpt) override final;
	virtual std::unique_ptr<Transaction> Finalize(const SessionToken& token, const Slate& slate) override final;

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB* m_pWalletDB;
	SessionManager m_sessionManager;
};