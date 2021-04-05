#pragma once

#include <Common/Secure.h>
#include <uuid.h>

// Forward Declarations
class SqliteDB;

class AccountsTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const int previousVersion);

	static int GetNextChildIndex(SqliteDB& database, std::string parentPath);
	static void UpdateNextChildIndex(SqliteDB& database, const std::string parentPath, int index);

	static int GetCurrentAddressIndex(SqliteDB& database, std::string parentPath);
	static void UpdateCurrentAddressIndex(SqliteDB& database, std::string parentPath, int index);
};