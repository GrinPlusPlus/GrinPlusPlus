#pragma once

#include <Wallet/WalletDB/WalletDB.h>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

using namespace rocksdb;

class WalletDB : public IWalletDB
{
public:
	WalletDB(const Config& config);

	void Open();
	void Close();

	virtual bool CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed) override final;

	virtual std::unique_ptr<EncryptedSeed> LoadWalletSeed(const std::string& username) const override final;
	virtual KeyChainPath GetNextChildPath(const std::string& username, const KeyChainPath& parentPath) override final;

	virtual std::unique_ptr<SlateContext> LoadSlateContext(const std::string& username, const CBigInteger<32>& masterSeed, const uuids::uuid& slateId) const override final;
	virtual bool SaveSlateContext(const std::string& username, const CBigInteger<32>& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext) override final;

	virtual bool AddOutputs(const std::string& username, const CBigInteger<32>& masterSeed, const std::vector<OutputData>& outputs) override final;
	virtual std::vector<OutputData> GetOutputs(const std::string& username, const CBigInteger<32>& masterSeed) const override final;
	virtual std::vector<OutputData> GetOutputsByStatus(const std::string& username, const CBigInteger<32>& masterSeed, const EOutputStatus outputStatus) const override final;

private:
	static std::string GetUsernamePrefix(const std::string& username);
	static Slice CombineKeyWithUsername(const std::string& username, const std::string& key);

	const Config& m_config;

	DB* m_pDatabase;
	ColumnFamilyHandle* m_pDefaultHandle;
	ColumnFamilyHandle* m_pSeedHandle;
	ColumnFamilyHandle* m_pNextChildHandle;
	ColumnFamilyHandle* m_pLogHandle;
	ColumnFamilyHandle* m_pSlateHandle;
	ColumnFamilyHandle* m_pTxHandle;
	ColumnFamilyHandle* m_pOutputHandle;
};