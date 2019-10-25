#pragma once

#include <Common/Secure.h>
#include <Wallet/WalletTx.h>
#include <libsqlite3/sqlite3.h>

class TransactionsTable
{
  public:
    static void CreateTable(sqlite3 &database);
    static void UpdateSchema(sqlite3 &database, const SecureVector &masterSeed, const int previousVersion);

    static void AddTransactions(sqlite3 &database, const SecureVector &masterSeed,
                                const std::vector<WalletTx> &transactions);
    static std::vector<WalletTx> GetTransactions(sqlite3 &database, const SecureVector &masterSeed);
};