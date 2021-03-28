#pragma once

#include "RocksDB.h"
#include "RocksDBTable.h"

#include <Database/DatabaseException.h>
#include <Common/Logger.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem.h>

class RocksDBFactory
{
public:
	//
	// tableNames - First table name is the default table, so must be empty
	//
    static std::unique_ptr<RocksDB> Open(const fs::path& dbPath, const std::vector<rocksdb::ColumnFamilyDescriptor>& tableNames)
    {
		fs::create_directories(dbPath);

		rocksdb::Options options;
		options.IncreaseParallelism();
		options.create_if_missing = true;
		options.compression = rocksdb::kNoCompression;

		std::vector<rocksdb::ColumnFamilyDescriptor> columnDescriptors = CreateDescriptors(options, dbPath, tableNames);

		rocksdb::OptimisticTransactionDB* pTransactionDB = nullptr;
		std::vector<rocksdb::ColumnFamilyHandle*> columnHandles;
		rocksdb::Status status = rocksdb::OptimisticTransactionDB::Open(options, dbPath.u8string(), columnDescriptors, &columnHandles, &pTransactionDB);
		if (!status.ok())
		{
			throw DATABASE_EXCEPTION_F("DB::Open failed with error {}", status.getState());
		}

		std::vector<RocksDBTable> tables = CreateTables(pTransactionDB, tableNames, columnHandles);

		return std::make_unique<RocksDB>(std::shared_ptr<rocksdb::OptimisticTransactionDB>(pTransactionDB), tables);
    }

private:
	static std::vector<rocksdb::ColumnFamilyDescriptor> CreateDescriptors(
		const rocksdb::Options& options,
		const fs::path& dbPath,
		const std::vector<rocksdb::ColumnFamilyDescriptor>& tableNames)
	{
		std::vector<std::string> columnFamilies;
		rocksdb::Status status = rocksdb::OptimisticTransactionDB::ListColumnFamilies(options, dbPath.u8string(), &columnFamilies);

		std::vector<rocksdb::ColumnFamilyDescriptor> columnDescriptors({ tableNames[0] });

		if (status.ok())
		{
			if (columnFamilies.size() > tableNames.size())
			{
				LOG_ERROR_F("Found more tables than expected for {}. Did a downgrade occur?", dbPath);
				throw DATABASE_EXCEPTION("Too many db tables found.");
			}

			for (size_t i = 1; i < columnFamilies.size(); i++)
			{
				if (tableNames.size() > i && tableNames[i].name != columnFamilies[i])
				{
					LOG_ERROR_F("Expected table {} but found {}", tableNames[i].name, columnFamilies[i]);
					throw DATABASE_EXCEPTION("Unexpected db table found.");
				}

				columnDescriptors.push_back(tableNames[i]);
			}
		}
		else
		{
			LOG_INFO_F("Database does not exist at ({}). Creating it now", dbPath);
		}

		return columnDescriptors;
	}

	static std::vector<RocksDBTable> CreateTables(
		rocksdb::OptimisticTransactionDB* pTxDB,
		const std::vector<rocksdb::ColumnFamilyDescriptor>& tableNames,
		const std::vector<rocksdb::ColumnFamilyHandle*>& columnHandles)
	{
		std::vector<RocksDBTable> tables;

		for (size_t i = 0; i < tableNames.size(); i++)
		{
			if (columnHandles.size() > i)
			{
				rocksdb::ColumnFamilyHandle* pHandle = columnHandles[i];
				tables.push_back(RocksDBTable(tableNames[i].name, std::shared_ptr<rocksdb::ColumnFamilyHandle>(pHandle)));
			}
			else
			{
				rocksdb::ColumnFamilyOptions columnFamilyOptions;
				columnFamilyOptions.OptimizeForPointLookup(1024); // TODO: Pass in options
				rocksdb::ColumnFamilyHandle* pHandle;

				rocksdb::Status status = pTxDB->GetBaseDB()->CreateColumnFamily(tableNames[i].options, tableNames[i].name, &pHandle);
				if (!status.ok())
				{
					LOG_ERROR_F("CreateColumnFamily failed for table {} with error: {}", tableNames[i].name, status.getState());
					throw DATABASE_EXCEPTION_F("CreateColumnFamily failed for table {} with error: {}", tableNames[i].name, status.getState());
				}

				tables.push_back(RocksDBTable(tableNames[i].name, std::shared_ptr<rocksdb::ColumnFamilyHandle>(pHandle)));
			}
		}

		return tables;
	}
};