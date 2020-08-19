#pragma once

// Forward Declarations
class SqliteDB;

class VersionTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const int previousVersion);
	static int GetCurrentVersion(SqliteDB& database);

private:
	static bool DoesTableExist(SqliteDB& database);
};