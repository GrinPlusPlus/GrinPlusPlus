#pragma once

#include <Common/Secure.h>
#include <Wallet/WalletTx.h>

// Forward Declarations
class SqliteDB;

class TransactionsTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const SecureVector& masterSeed, const int previousVersion);

	static void AddTransactions(SqliteDB& database, const SecureVector& masterSeed, const std::vector<WalletTx>& transactions);
	static std::vector<WalletTx> GetTransactions(SqliteDB& database, const SecureVector& masterSeed);
	static std::unique_ptr<WalletTx> GetTransactionById(SqliteDB& database, const SecureVector& masterSeed, const uint32_t walletTxId);
};