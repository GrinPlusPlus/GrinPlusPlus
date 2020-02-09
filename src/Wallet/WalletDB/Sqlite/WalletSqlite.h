#pragma once

#include "../UserMetadata.h"
#include "SqliteTransaction.h"

#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/WalletDB/Models/SlateContextEntity.h>
#include <libsqlite3/sqlite3.h>
#include <unordered_map>

class WalletSqlite : public IWalletDB
{
public:
	explicit WalletSqlite(const fs::path& walletDirectory, const std::string& username, sqlite3* pDatabase)
		: m_walletDirectory(walletDirectory), m_username(username), m_pDatabase(pDatabase), m_pTransaction(nullptr)
	{
	
	}

	virtual ~WalletSqlite() { sqlite3_close(m_pDatabase); }

	virtual void Commit() override final;
	virtual void Rollback() noexcept override final;
	virtual void OnInitWrite() override final;
	virtual void OnEndWrite() override final;

	virtual KeyChainPath GetNextChildPath(const KeyChainPath& parentPath) override final;

	virtual std::unique_ptr<SlateContextEntity> LoadSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId) const override final;
	virtual void SaveSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContextEntity& slateContext) override final;

	virtual void AddOutputs(const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs) override final;
	virtual std::vector<OutputDataEntity> GetOutputs(const SecureVector& masterSeed) const override final;

	virtual void AddTransaction(const SecureVector& masterSeed, const WalletTx& walletTx) override final;
	virtual std::vector<WalletTx> GetTransactions(const SecureVector& masterSeed) const override final;
	virtual std::unique_ptr<WalletTx> GetTransactionById(const SecureVector& masterSeed, const uint32_t walletTxId) const override final;

	virtual uint32_t GetNextTransactionId() override final;
	virtual uint64_t GetRefreshBlockHeight() const override final;
	virtual void UpdateRefreshBlockHeight(const uint64_t refreshBlockHeight) override final;
	virtual uint64_t GetRestoreLeafIndex() const override final;
	virtual void UpdateRestoreLeafIndex(const uint64_t lastLeafIndex) override final;

private:
	UserMetadata GetMetadata() const;
	void SaveMetadata(const UserMetadata& userMetadata);

	fs::path m_walletDirectory;
	std::string m_username;
	sqlite3* m_pDatabase;
	std::unique_ptr<SqliteTransaction> m_pTransaction;
};
