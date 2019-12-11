#pragma once

#include <libsqlite3/sqlite3.h>
#include <Common/Secure.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>

class OutputsTable
{
public:
	static void CreateTable(sqlite3& database);
	static void UpdateSchema(sqlite3& database, const SecureVector& masterSeed, const int previousVersion);

	static void AddOutputs(sqlite3& database, const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs);
	static std::vector<OutputDataEntity> GetOutputs(sqlite3& database, const SecureVector& masterSeed);

private:
	static void AddOutputs(sqlite3& database, const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs, const std::string& tableName);
	static std::vector<OutputDataEntity> GetOutputs(sqlite3& database, const SecureVector& masterSeed, const int version);
};