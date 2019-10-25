#pragma once

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>
#include <libsqlite3/sqlite3.h>

class SqliteTransaction
{
  public:
    SqliteTransaction(sqlite3 &database) : m_database(database), m_open(false)
    {
    }

    ~SqliteTransaction()
    {
        if (m_open)
        {
            Rollback();
        }
    }

    void Begin()
    {
        if (m_open)
        {
            LoggerAPI::LogError(StringUtil::Format("SqliteTransaction::Begin - Tried to begin a transaction twice"));
            throw WALLET_STORE_EXCEPTION("Tried to begin a transaction twice.");
        }

        char *error = nullptr;
        if (sqlite3_exec(&m_database, "BEGIN;", NULL, NULL, &error) != SQLITE_OK)
        {
            LoggerAPI::LogError(StringUtil::Format("SqliteTransaction::Begin - Failed with error: %s", error));
            sqlite3_free(error);
            throw WALLET_STORE_EXCEPTION("Failed to begin transaction");
        }

        m_open = true;
    }

    void Commit()
    {
        if (!m_open)
        {
            LoggerAPI::LogError(StringUtil::Format("SqliteTransaction::Commit - Tried to commit a closed transaction"));
            throw WALLET_STORE_EXCEPTION("Tried to commit a closed transaction.");
        }

        m_open = false;

        char *error = nullptr;
        if (sqlite3_exec(&m_database, "COMMIT;", NULL, NULL, &error) != SQLITE_OK)
        {
            LoggerAPI::LogError(StringUtil::Format("SqliteTransaction::Commit - Failed with error: %s", error));
            sqlite3_free(error);
            throw WALLET_STORE_EXCEPTION("Failed to commit transaction");
        }
    }

    void Rollback()
    {
        if (!m_open)
        {
            LoggerAPI::LogError(
                StringUtil::Format("SqliteTransaction::Rollback - Tried to rollback a closed transaction"));
            throw WALLET_STORE_EXCEPTION("Tried to rollback a closed transaction.");
        }

        m_open = false;

        char *error = nullptr;
        if (sqlite3_exec(&m_database, "ROLLBACK;", NULL, NULL, &error) != SQLITE_OK)
        {
            LoggerAPI::LogError(StringUtil::Format("SqliteTransaction::Rollback - Failed with error: %s", error));
            sqlite3_free(error);
            throw WALLET_STORE_EXCEPTION("Failed to rollback transaction");
        }
    }

  private:
    sqlite3 &m_database;
    bool m_open;
};