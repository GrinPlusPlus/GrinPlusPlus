#include <Wallet/WalletDB/WalletDB.h>

#ifndef __linux__ 
#include "WalletRocksDB.h"
#endif

#include "WalletSqlite.h"

namespace WalletDBAPI
{
	//
	// Opens the wallet database and returns an instance of IWalletDB.
	//
	WALLET_DB_API IWalletDB* OpenWalletDB(const Config& config)
	{
#ifdef __linux__
		WalletSqlite* pWalletDB = new WalletSqlite(config);
		pWalletDB->Open();

		return pWalletDB;
#else
		if (config.GetWalletConfig().GetDatabaseType() == "SQLITE")
		{
			WalletSqlite* pWalletDB = new WalletSqlite(config);
			pWalletDB->Open();

			return pWalletDB;
		}
		else
		{
			WalletRocksDB* pWalletDB = new WalletRocksDB(config);
			pWalletDB->Open();

			return pWalletDB;
		}
#endif
	}

	//
	// Closes the wallet database and cleans up the memory of IWalletDB.
	//
	WALLET_DB_API void CloseWalletDB(IWalletDB* pWalletDB)
	{
#ifdef __linux__
		((WalletSqlite*)pWalletDB)->Close();
		delete (WalletSqlite*)pWalletDB;
#else
		WalletRocksDB* pRocksDB = dynamic_cast<WalletRocksDB*>(pWalletDB);
		if (pRocksDB != nullptr)
		{
			pRocksDB->Close();
			delete pRocksDB;
		}
		else
		{
			((WalletSqlite*)pWalletDB)->Close();
			delete (WalletSqlite*)pWalletDB;
		}
#endif
	}
}