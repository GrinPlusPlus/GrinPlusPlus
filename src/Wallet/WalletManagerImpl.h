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

	bool ShouldReuseAddresses() final;

	Locked<Wallet> GetWallet(const SessionToken& token) final;

	CreateWalletResponse InitializeNewWallet(
		const CreateWalletCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;

	LoginResponse RestoreFromSeed(
		const RestoreWalletCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;

	SecureString GetSeedWords(const GetSeedPhraseCriteria& criteria) final;
	void CheckForOutputs(const SessionToken& token, const bool fromGenesis) final;

	std::optional<TorAddress> AddTorListener(
		const SessionToken& token,
		const KeyChainPath& path,
		const TorProcess::Ptr& pTorProcess
	);

	std::optional<TorAddress> CheckTorListener(
		const SessionToken& token,
		const TorProcess::Ptr& pTorProcess
	);

	LoginResponse Login(
		const LoginCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) final;
	void Logout(const SessionToken& token) final;
	void DeleteWallet(const GrinStr& username, const SecureString& password) final;
	void ChangePassword(
		const GrinStr& username,
		const SecureString& currentPassword,
		const SecureString& newPassword
	) final;

	std::vector<GrinStr> GetAllAccounts() const final;

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

private:
	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	std::shared_ptr<IWalletStore> m_pWalletStore;
	Locked<SessionManager> m_sessionManager;
};