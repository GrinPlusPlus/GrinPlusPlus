#pragma once

#include "../../UserMetadata.h"
#include "../SqliteDB.h"

class MetadataTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const int previousVersion);

	static UserMetadata GetMetadata(SqliteDB& database);
	static void SaveMetadata(SqliteDB& database, const UserMetadata& userMetadata);
};