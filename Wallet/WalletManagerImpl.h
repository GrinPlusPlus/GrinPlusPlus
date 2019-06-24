#pragma once

#include "SessionManager.h"

#include <Wallet/WalletManager.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <map>

class WalletManager : public IWalletManager
{
public:
	WalletManager(const Config& config, INodeClient& nodeClient, IWalletDB* pWalletDB);
	~WalletManager();

	virtual std::optional<std::pair<SecureString, SessionToken>> InitializeNewWallet(const std::string& username, const SecureString& password) override final;
	virtual std::optional<SessionToken> RestoreFromSeed(const std::string& username, const SecureString& password, const SecureString& walletWords) override final;
	virtual bool CheckForOutputs(const SessionToken& token, const bool fromGenesis) override final;
	virtual SecretKey GetGrinboxAddress(const SessionToken& token) const override final;

	virtual std::unique_ptr<SessionToken> Login(const std::string& username, const SecureString& password) override final;
	virtual void Logout(const SessionToken& token) override final;
	virtual std::vector<std::string> GetAllAccounts() const override final;

	virtual WalletSummary GetWalletSummary(const SessionToken& token) override final;
	virtual std::vector<WalletTxDTO> GetTransactions(const SessionToken& token) override final;
	virtual std::vector<WalletOutputDTO> GetOutputs(const SessionToken& token, const bool includeSpent, const bool includeCanceled) override final;

	virtual uint64_t EstimateFee(const SessionToken& token, const uint64_t amountToSend, const uint64_t feeBase, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs) override final;
	virtual std::unique_ptr<Slate> Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs) override final;
	virtual std::unique_ptr<Slate> Receive(const SessionToken& token, const Slate& slate, const std::optional<std::string>& messageOpt) override final;
	virtual std::unique_ptr<Slate> Finalize(const SessionToken& token, const Slate& slate) override final;
	virtual bool PostTransaction(const SessionToken& token, const Transaction& transaction) override final;
	virtual bool RepostByTxId(const SessionToken& token, const uint32_t walletTxId) override final;

	virtual bool CancelByTxId(const SessionToken& token, const uint32_t walletTxId) override final;

private:
	const Config& m_config;
	INodeClient& m_nodeClient;
	IWalletDB* m_pWalletDB;
	SessionManager m_sessionManager;
};