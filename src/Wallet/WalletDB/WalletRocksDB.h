#pragma once

#include "UserMetadata.h"

#include <Config/Config.h>
#include <Wallet/EncryptedSeed.h>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

using namespace rocksdb;

//
// WalletRocksDB is deprecated, and only remains to help migrate existing wallets to SQLITE.
//
class WalletRocksDB
{
public:
	virtual ~WalletRocksDB();

	static std::shared_ptr<WalletRocksDB> Open(const Config& config);

	std::vector<std::string> GetAccounts() const;
	std::unique_ptr<EncryptedSeed> LoadWalletSeed(const std::string& username) const;

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