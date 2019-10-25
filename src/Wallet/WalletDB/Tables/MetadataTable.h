#pragma once

#include "../UserMetadata.h"
#include <libsqlite3/sqlite3.h>

class MetadataTable
{
  public:
    static void CreateTable(sqlite3 &database);
    static void UpdateSchema(sqlite3 &database, const int previousVersion);

    static UserMetadata GetMetadata(sqlite3 &database);
    static void SaveMetadata(sqlite3 &database, const UserMetadata &userMetadata);
};