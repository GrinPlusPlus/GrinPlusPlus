#pragma once

#include "SessionManager.h"

#include <Wallet/WalletManager.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletStore.h>
#include <map>

class WalletManager : public IWalletManager
{
public:
	WalletManager(const Config& config, INodeClientPtr nodeClient, std::shared_ptr<IWalletStore> pWalletStore);
	virtual ~WalletManager() = default;

	std::pair<SecureString, SessionToken> InitializeNewWallet(const CreateWalletCriteria& criteria) final;

	std::optional<SessionToken> RestoreFromSeed(
		const std::string& username,
		const SecureString& password,
		const SecureString& walletWords
	) final;

	SecureString GetSeedWords(const SessionToken& token) final;
	void CheckForOutputs(const SessionToken& token, const bool fromGenesis) final;
	std::optional<TorAddress> GetTorAddress(const SessionToken& token) const final;
	std::optional<TorAddress> AddTorListener(const SessionToken& token, const KeyChainPath& path);
	uint16_t GetListenerPort(const SessionToken& token) const final;

	SessionToken Login(const std::string& username, const SecureString& password) final;
	void Logout(const SessionToken& token) final;
	void DeleteWallet(const std::string& username, const SecureString& password) final;
	void ChangePassword(
		const std::string& username,
		const SecureString& currentPassword,
		const SecureString& newPassword
	) final;

	std::vector<std::string> GetAllAccounts() const final;

	WalletSummaryDTO GetWalletSummary(const SessionToken& token) final;
	std::vector<WalletTxDTO> GetTransactions(const SessionToken& token) final;
	std::vector<WalletOutputDTO> GetOutputs(
		const SessionToken& token,
		const bool includeSpent,
		const bool includeCanceled
	) final;

	FeeEstimateDTO EstimateFee(
		const SessionToken& token,
		const uint64_t amountToSend,
		const uint64_t feeBase,
		const SelectionStrategyDTO& strategy,
		const uint8_t numChangeOutputs
	) final;

	Slate Send(const SendCriteria& sendCriteria) final;
	Slate Receive(const ReceiveCriteria& receiveCriteria) final;
	Slate Finalize(const FinalizeCriteria& finalizeCriteria) final;

	bool PostTransaction(const Transaction& transaction, const PostMethodDTO& postMethod) final;
	bool RepostByTxId(const SessionToken& token, const uint32_t walletTxId) final;

	void CancelByTxId(const SessionToken& token, const uint32_t walletTxId) final;

private:
	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	std::shared_ptr<IWalletStore> m_pWalletStore;
	Locked<SessionManager> m_sessionManager;
};