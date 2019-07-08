#pragma once

#include "UserMetadata.h"

#include <Wallet/WalletDB/WalletDB.h>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

using namespace rocksdb;

class WalletRocksDB : public IWalletDB
{
public:
	WalletRocksDB(const Config& config);

	void Open();
	void Close();

	virtual std::vector<std::string> GetAccounts() const override final;

	virtual bool OpenWallet(const std::string& username) override final;
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
	std::unique_ptr<UserMetadata> GetMetadata(const std::string& username) const;
	bool SaveMetadata(const std::string& username, const UserMetadata& userMetadata);

	static std::string GetUsernamePrefix(const std::string& username);
	static std::string CombineKeyWithUsername(const std::string& username, const std::string& key);
	static SecretKey CreateSecureKey(const SecureVector& masterSeed, const std::string& dataType);

	static std::vector<unsigned char> Encrypt(const SecureVector& masterSeed, const std::string& dataType, const SecureVector& bytes);
	static SecureVector Decrypt(const SecureVector& masterSeed, const std::string& dataType, const std::vector<unsigned char>& encrypted);

	const Config& m_config;

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