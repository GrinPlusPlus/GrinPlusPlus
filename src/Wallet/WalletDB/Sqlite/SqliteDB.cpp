#include "SqliteDB.h"

#include <libsqlite3/sqlite3.h>
#include <Common/Logger.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/HexUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>

/**
 * 
 * SqliteDB::Statement
 * 
 */

SqliteDB::Statement::~Statement()
{
	if (!m_finalized && sqlite3_finalize(m_pStatement) != SQLITE_OK) {
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(m_pDatabase));
    }
}

bool SqliteDB::Statement::Step()
{
	return sqlite3_step(m_pStatement) == SQLITE_ROW;
}

void SqliteDB::Statement::Finalize()
{
    m_finalized = true;
	if (sqlite3_finalize(m_pStatement) != SQLITE_OK) {
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(m_pDatabase));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}
}

bool SqliteDB::Statement::IsColumnNull(const int col) const
{
	return sqlite3_column_type(m_pStatement, col) == SQLITE_NULL;
}

int SqliteDB::Statement::GetColumnInt(const int col) const
{
    return sqlite3_column_int(m_pStatement, col);
}

int64_t SqliteDB::Statement::GetColumnInt64(const int col) const
{
    return sqlite3_column_int64(m_pStatement, col);
}

std::vector<uint8_t> SqliteDB::Statement::GetColumnBytes(const int col) const
{
    const int size = sqlite3_column_bytes(m_pStatement, col);
    const unsigned char* pBytes = (const unsigned char*)sqlite3_column_blob(m_pStatement, col);
    return std::vector<uint8_t>{ pBytes, pBytes + size };
}

std::string SqliteDB::Statement::GetColumnString(const int col) const
{
	return (const char*)sqlite3_column_text(m_pStatement, col);
}

/**
 * 
 * SqliteDB
 * 
 */

SqliteDB::~SqliteDB()
{
	sqlite3_close(m_pDatabase);
}

SqliteDB::Ptr SqliteDB::Open(const fs::path& db_path, const std::string& username)
{
	sqlite3* pDatabase = nullptr;

    if (sqlite3_open(db_path.u8string().c_str(), &pDatabase) != SQLITE_OK) {
        WALLET_ERROR_F("Failed to open wallet.db for user ({}) with error ({})", username, sqlite3_errmsg(pDatabase));
        sqlite3_close(pDatabase);
        throw WALLET_STORE_EXCEPTION("Failed to open wallet.db");
    }

    return std::make_shared<SqliteDB>(pDatabase, username);
}

void SqliteDB::Execute(const std::string& command)
{
	char* error = nullptr;
	if (sqlite3_exec(m_pDatabase, command.c_str(), NULL, NULL, &error) != SQLITE_OK) {
		WALLET_ERROR_F("Failed to execute command: {}. Error: {}", command, error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to execute command.");
	}
}

void SqliteDB::Update(const std::string& statement, const std::vector<IParameter::UPtr>& parameters)
{
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_pDatabase, statement.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(m_pDatabase));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

    SqliteDB::Statement::UPtr pStatement = std::make_unique<SqliteDB::Statement>(m_pDatabase, stmt);

    int index = 1;
    for (const auto& pParam : parameters)
    {
        pParam->Bind(stmt, index++);
    }

    pStatement->Step();
    pStatement->Finalize();
}

SqliteDB::Statement::UPtr SqliteDB::Query(const std::string& query, const std::vector<IParameter::UPtr>& parameters)
{
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_pDatabase, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(m_pDatabase));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

    int index = 1;
    for (const auto& pParam : parameters)
    {
        pParam->Bind(stmt, index++);
    }

    return std::make_unique<SqliteDB::Statement>(m_pDatabase, stmt);
}

std::string SqliteDB::GetError() const
{
    return std::string(sqlite3_errmsg(m_pDatabase));
}

/**
 * 
 * SqliteDB::IParameter
 * 
 */

void TextParameter::Bind(sqlite3_stmt* stmt, const int index) const
{
	if (sqlite3_bind_text(stmt, index, m_value.c_str(), (int)m_value.size(), NULL) != SQLITE_OK) {
		WALLET_ERROR_F("Failed to bind: {}", m_value);
		throw WALLET_STORE_EXCEPTION("Failed to bind.");
    }
}

void IntParameter::Bind(sqlite3_stmt* stmt, const int index) const
{
	if (sqlite3_bind_int(stmt, index, m_value) != SQLITE_OK) {
		WALLET_ERROR_F("Failed to bind: {}", m_value);
		throw WALLET_STORE_EXCEPTION("Failed to bind.");
    }
}

void BlobParameter::Bind(sqlite3_stmt* stmt, const int index) const
{
	if (sqlite3_bind_blob(stmt, index, (const void*)m_blob.data(), (int)m_blob.size(), NULL) != SQLITE_OK) {
		WALLET_ERROR_F("Failed to bind: {}", HexUtil::ConvertToHex(m_blob));
		throw WALLET_STORE_EXCEPTION("Failed to bind.");
    }
}

void NullParameter::Bind(sqlite3_stmt* stmt, const int index) const
{
	if (sqlite3_bind_null(stmt, index) != SQLITE_OK) {
		WALLET_ERROR("Failed to bind null");
		throw WALLET_STORE_EXCEPTION("Failed to bind.");
    }
}