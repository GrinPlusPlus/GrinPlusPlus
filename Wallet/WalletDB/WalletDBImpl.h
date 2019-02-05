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

	virtual bool SaveSlateContext(const std::string& username, const uuids::uuid& slateId, const SlateContext& slateContext) override final;

	virtual bool AddOutputs(const std::string& username, const std::vector<OutputData>& outputs) override final;
	virtual std::vector<OutputData> GetOutputs(const std::string& username) const override final;

private:
	const Config& m_config;

	DB* m_pDatabase;
	ColumnFamilyHandle* m_pDefaultHandle;
	ColumnFamilyHandle* m_pNextChildHandle;
	ColumnFamilyHandle* m_pLogHandle;
	ColumnFamilyHandle* m_pSlateHandle;
	ColumnFamilyHandle* m_pTxHandle;
	ColumnFamilyHandle* m_pOutputHandle;
};