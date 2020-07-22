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
	~WalletManager();

	CreateWalletResponse InitializeNewWallet(
		const CreateWalletCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;

	LoginResponse RestoreFromSeed(
		const RestoreWalletCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;

	SecureString GetSeedWords(const SessionToken& token) final;
	SecureString GetSeedWords(const GetSeedPhraseCriteria& criteria) final;
	void CheckForOutputs(const SessionToken& token, const bool fromGenesis) final;

	SlatepackAddress GetSlatepackAddress(const SessionToken& token) const final;
	std::optional<TorAddress> GetTorAddress(const SessionToken& token) const final;
	std::optional<TorAddress> AddTorListener(
		const SessionToken& token,
		const KeyChainPath& path,
		const TorProcess::Ptr& pTorProcess
	);
	uint16_t GetListenerPort(const SessionToken& token) const final;

	LoginResponse Login(
		const LoginCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;
	void Logout(const SessionToken& token) final;
	void DeleteWallet(const std::string& username, const SecureString& password) final;
	void ChangePassword(
		const std::string& username,
		const SecureString& currentPassword,
		const SecureString& newPassword
	) final;

	std::vector<std::string> GetAllAccounts() const final;

	WalletSummaryDTO GetWalletSummary(const SessionToken& token) final;
	WalletBalanceDTO GetBalance(const SessionToken& token) final;
	std::vector<WalletTxDTO> GetTransactions(const ListTxsCriteria& criteria) final;
	std::vector<WalletOutputDTO> GetOutputs(
		const SessionToken& token,
		const bool includeSpent,
		const bool includeCanceled
	) final;

	FeeEstimateDTO EstimateFee(const EstimateFeeCriteria& criteria) final;

	Slate Send(const SendCriteria& sendCriteria) final;
	Slate Receive(const ReceiveCriteria& receiveCriteria) final;
	Slate Finalize(const FinalizeCriteria& finalizeCriteria, const TorProcess::Ptr& pTorProcess) final;

	bool PostTransaction(
		const Transaction& transaction,
		const PostMethodDTO& postMethod,
		const TorProcess::Ptr& pTorProcess
	) final;
	bool RepostTx(
		const RepostTxCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;

	void CancelByTxId(const SessionToken& token, const uint32_t walletTxId) final;

	BuildCoinbaseResponse BuildCoinbase(const BuildCoinbaseCriteria& criteria) final;

	SlatepackMessage DecryptSlatepack(
		const SessionToken& token,
		const std::string& armoredSlatepack
	) const final;

private:
	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	std::shared_ptr<IWalletStore> m_pWalletStore;
	Locked<SessionManager> m_sessionManager;
};