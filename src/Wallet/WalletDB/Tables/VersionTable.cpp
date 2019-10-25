#include "VersionTable.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Wallet/OutputData.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void VersionTable::CreateTable(sqlite3 &database)
{
    std::string tableCreation = "create table version(schema_version INTEGER PRIMARY KEY);";
    tableCreation += "insert into version(schema_version) values (1)";

    char *error = nullptr;
    if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
    {
        LoggerAPI::LogError(
            StringUtil::Format("VersionTable::CreateTable - Create version table failed with error: %s", error));
        sqlite3_free(error);
        throw WALLET_STORE_EXCEPTION("Failed to create version table.");
    }
}

int VersionTable::GetCurrentVersion(sqlite3 &database)
{
    if (!DoesTableExist(database))
    {
        return 0;
    }

    sqlite3_stmt *stmt = nullptr;
    const std::string query = "SELECT schema_version FROM version";
    if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format("VersionTable::GetCurrentVersion - Error while compiling sql: %s",
                                               sqlite3_errmsg(&database)));
        sqlite3_finalize(stmt);
        throw WALLET_STORE_EXCEPTION("Error compiling statement.");
    }

    int version = 0;
    int ret_code = sqlite3_step(stmt);
    if (ret_code == SQLITE_ROW)
    {
        version = sqlite3_column_int(stmt, 0);
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format("VersionTable::GetCurrentVersion - Error finalizing statement: %s",
                                               sqlite3_errmsg(&database)));
        throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
    }

    return version;
}

bool VersionTable::DoesTableExist(sqlite3 &database)
{
    sqlite3_stmt *stmt = nullptr;
    const std::string query = "PRAGMA table_info(version)";
    if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format("VersionTable::DoesTableExist - Error while compiling sql: %s",
                                               sqlite3_errmsg(&database)));
        sqlite3_finalize(stmt);
        throw WALLET_STORE_EXCEPTION("Error compiling statement.");
    }

    bool exists = false;
    int ret_code = sqlite3_step(stmt);
    if (ret_code == SQLITE_ROW)
    {
        exists = true;
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        LoggerAPI::LogError(StringUtil::Format("VersionTable::DoesTableExist - Error finalizing statement: %s",
                                               sqlite3_errmsg(&database)));
        throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
    }

    return exists;
}