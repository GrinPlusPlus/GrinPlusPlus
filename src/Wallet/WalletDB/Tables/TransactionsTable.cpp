#include "TransactionsTable.h"
#include "../WalletEncryptionUtil.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

// TABLE: transactions
// id: INTEGER PRIMARY KEY
// slate_id: TEXT
// encrypted: BLOB NOT NULL
void TransactionsTable::CreateTable(sqlite3 &database)
{
    const std::string tableCreation =
        "create table transactions(id INTEGER PRIMARY KEY, slate_id TEXT, encrypted BLOB NOT NULL);";
    char *error = nullptr;
    if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format(
            "TransactionsTable::CreateTable - Create transactions table failed with error: %s", error));
        sqlite3_free(error);
        throw WALLET_STORE_EXCEPTION("Error creating transactions table.");
    }
}

void TransactionsTable::UpdateSchema(sqlite3 &database, const SecureVector &masterSeed, const int previousVersion)
{
    return;
}

void TransactionsTable::AddTransactions(sqlite3 &database, const SecureVector &masterSeed,
                                        const std::vector<WalletTx> &transactions)
{
    for (const WalletTx &walletTx : transactions)
    {
        sqlite3_stmt *stmt = nullptr;
        std::string insert = "insert into transactions(id, slate_id, encrypted) values(?, ?, ?)";
        insert += " ON CONFLICT(id) DO UPDATE SET slate_id=excluded.slate_id, encrypted=excluded.encrypted";
        if (sqlite3_prepare_v2(&database, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
        {
            LoggerAPI::LogError(StringUtil::Format("TransactionsTable::AddTransactions - Error while compiling sql: %s",
                                                   sqlite3_errmsg(&database)));
            sqlite3_finalize(stmt);
            throw WALLET_STORE_EXCEPTION("Error compiling statement.");
        }

        sqlite3_bind_int(stmt, 1, (int)walletTx.GetId());

        if (walletTx.GetSlateId().has_value())
        {
            const std::string slateId = uuids::to_string(walletTx.GetSlateId().value());
            sqlite3_bind_text(stmt, 2, slateId.c_str(), (int)slateId.size(), NULL);
        }
        else
        {
            sqlite3_bind_null(stmt, 2);
        }

        Serializer serializer;
        walletTx.Serialize(serializer);
        const std::vector<unsigned char> encrypted =
            WalletEncryptionUtil::Encrypt(masterSeed, "WALLET_TX", serializer.GetSecureBytes());
        sqlite3_bind_blob(stmt, 3, (const void *)encrypted.data(), (int)encrypted.size(), NULL);

        sqlite3_step(stmt);

        if (sqlite3_finalize(stmt) != SQLITE_OK)
        {
            LoggerAPI::LogError(StringUtil::Format(
                "TransactionsTable::AddTransactions - Error finalizing statement: %s", sqlite3_errmsg(&database)));
            throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
        }
    }
}

std::vector<WalletTx> TransactionsTable::GetTransactions(sqlite3 &database, const SecureVector &masterSeed)
{
    sqlite3_stmt *stmt = nullptr;
    const std::string query = "select encrypted from transactions";
    if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format("TransactionsTable::GetTransactions - Error while compiling sql: %s",
                                               sqlite3_errmsg(&database)));
        sqlite3_finalize(stmt);
        throw WALLET_STORE_EXCEPTION("Error compiling statement.");
    }

    std::vector<WalletTx> transactions;

    int ret_code = 0;
    while ((ret_code = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const int encryptedSize = sqlite3_column_bytes(stmt, 0);
        const unsigned char *pEncrypted = (const unsigned char *)sqlite3_column_blob(stmt, 0);
        std::vector<unsigned char> encrypted(pEncrypted, pEncrypted + encryptedSize);
        const SecureVector decrypted = WalletEncryptionUtil::Decrypt(masterSeed, "WALLET_TX", encrypted);
        const std::vector<unsigned char> decryptedUnsafe(decrypted.begin(), decrypted.end());

        ByteBuffer byteBuffer(decryptedUnsafe);
        transactions.emplace_back(WalletTx::Deserialize(byteBuffer));
    }

    if (ret_code != SQLITE_DONE)
    {
        LoggerAPI::LogError(StringUtil::Format("TransactionsTable::GetTransactions - Error while performing sql: %s",
                                               sqlite3_errmsg(&database)));
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format("TransactionsTable::GetTransactions - Error finalizing statement: %s",
                                               sqlite3_errmsg(&database)));
        throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
    }

    return transactions;
}