#include <Wallet/WalletDB/WalletDB.h>

#include "Sqlite/SqliteStore.h"
#include <Common/Logger.h>
#include <Wallet/WalletDB/WalletStore.h>
#include <Wallet/WalletDB/WalletStoreException.h>

namespace WalletDBAPI
{
	//
	// Opens the wallet database and returns an instance of IWalletStore.
	//
	WALLET_DB_API std::shared_ptr<IWalletStore> OpenWalletDB(const Config& config)
	{
		return SqliteStore::Open(config);
	}
}