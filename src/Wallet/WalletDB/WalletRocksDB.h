#pragma once

#include "UserMetadata.h"

#include <Wallet/WalletDB/WalletDB.h>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

using namespace rocksdb;

//
// WalletRocksDB is deprecated, and only remains to help migrate existing wallets to SQLITE.
//
class WalletRocksDB : public IWalletDB
{
public:
	virtual ~WalletRocksDB();

	static std::shared_ptr<WalletRocksDB> Open(const Config& config);

	virtual std::vector<std::string> GetAccounts() const override final;

	virtual bool OpenWallet(const std::string& username, const SecureVector& masterSeed) override final;
	virtual bool CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed) override final;

	virtual std::unique_ptr<EncryptedSeed> LoadWalletSeed(const std::string& username) const override final;
	virtual KeyChainPath GetNextChildPath(const std::string& username, const KeyChainPath& parentPath) override final;

	virtual std::unique_ptr<SlateContext> LoadSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId) const override final;
	virtual bool SaveSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext) override final;

	virtual bool AddOutputs(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& outputs) override final;
	virtual std::vector<OutputData> GetOutputs(const std::string& username, const SecureVector& masterSeed) const override final;

	virtual bool AddTransaction(const std::string& username, const SecureVector& masterSeed, const WalletTx& walletTx) override final;
	virtual std::vector<WalletTx> GetTransactions(const std::string& username, const SecureVector& masterSeed) const override final;

	virtual uint32_t GetNextTransactionId(const std::string& username) override final;
	virtual uint64_t GetRefreshBlockHeight(const std::string& username) const override final;
	virtual bool UpdateRefreshBlockHeight(const std::string& username, const uint64_t refreshBlockHeight) override final;
	virtual uint64_t GetRestoreLeafIndex(const std::string& username) const override final;
	virtual bool UpdateRestoreLeafIndex(const std::string& username, const uint64_t lastLeafIndex) override final;

private:
	WalletRocksDB(
		DB* pDatabase,
		ColumnFamilyHandle* pDefaultHandle,
		ColumnFamilyHandle* pSeedHandle,
		ColumnFamilyHandle* pNextChildHandle,
		ColumnFamilyHandle* pLogHandle,
		ColumnFamilyHandle* pSlateHandle,
		ColumnFamilyHandle* pTxHandle,
		ColumnFamilyHandle* pOutputHandle,
		ColumnFamilyHandle* pUserMetadataHandle
	);

	DB* m_pDatabase = { nullptr };
	ColumnFamilyHandle* m_pDefaultHandle;
	ColumnFamilyHandle* m_pSeedHandle;
	ColumnFamilyHandle* m_pNextChildHandle;
	ColumnFamilyHandle* m_pLogHandle;
	ColumnFamilyHandle* m_pSlateHandle;
	ColumnFamilyHandle* m_pTxHandle;
	ColumnFamilyHandle* m_pOutputHandle;
	ColumnFamilyHandle* m_pUserMetadataHandle;
};