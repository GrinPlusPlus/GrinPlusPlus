#pragma once

#include <libsqlite3/sqlite3.h>

class VersionTable
{
public:
	static void CreateTable(sqlite3& database);
	static int GetCurrentVersion(sqlite3& database);

private:
	static bool DoesTableExist(sqlite3& database);
};