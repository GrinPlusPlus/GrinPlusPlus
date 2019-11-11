#pragma once

#include <libsqlite3/sqlite3.h>
#include <Common/Secure.h>
#include <Wallet/OutputData.h>

class OutputsTable
{
public:
	static void CreateTable(sqlite3& database);
	static void UpdateSchema(sqlite3& database, const SecureVector& masterSeed, const int previousVersion);

	static void AddOutputs(sqlite3& database, const SecureVector& masterSeed, const std::vector<OutputData>& outputs);
	static std::vector<OutputData> GetOutputs(sqlite3& database, const SecureVector& masterSeed);

private:
	static void AddOutputs(sqlite3& database, const SecureVector& masterSeed, const std::vector<OutputData>& outputs, const std::string& tableName);
	static std::vector<OutputData> GetOutputs(sqlite3& database, const SecureVector& masterSeed, const int version);
};