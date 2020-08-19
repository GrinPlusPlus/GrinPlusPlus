#pragma once

#include <Common/Secure.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>

#include "../SqliteDB.h"

class OutputsTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const SecureVector& masterSeed, const int previousVersion);

	static void AddOutputs(SqliteDB& database, const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs);
	static std::vector<OutputDataEntity> GetOutputs(SqliteDB& database, const SecureVector& masterSeed);

private:
	static void AddOutputs(SqliteDB& database, const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs, const std::string& tableName);
	static std::vector<OutputDataEntity> GetOutputs(SqliteDB& database, const SecureVector& masterSeed, const int version);
};