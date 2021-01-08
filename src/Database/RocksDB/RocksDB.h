#pragma once

#include "RocksDBTable.h"
#include "DBEntry.h"
#include <Core/Traits/Batchable.h>
#include <Database/DatabaseException.h>
#include <Common/Logger.h>
#include <Core/Serialization/ByteBuffer.h>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <filesystem.h>
#include <cassert>
#include <memory>
#include <vector>

class RocksDB : public Traits::IBatchable
{
public:
	RocksDB(const std::shared_ptr<rocksdb::OptimisticTransactionDB>& pTransactionDB, const std::vector<RocksDBTable>& tables)
		: m_pTransactionDB(pTransactionDB), m_tables(tables) { }

	virtual ~RocksDB()
	{
		m_pTransaction.reset();

		for (RocksDBTable& table : m_tables)
		{
			table.CloseHandle();
		}

		m_pTransactionDB->GetBaseDB()->Close();
		m_pTransactionDB.reset();
	}

	bool IsTransactional() const noexcept { return m_pTransaction != nullptr; }

	std::unique_ptr<rocksdb::Iterator> GetIterator(const RocksDBTable& table) const
	{
		return std::unique_ptr<rocksdb::Iterator>(m_pTransactionDB->NewIterator(rocksdb::ReadOptions(), table.GetHandle()));
	}

	std::unique_ptr<rocksdb::Iterator> GetIterator(const std::string& tableName) const
	{
		return GetIterator(GetTable(tableName));
	}

	template<typename T,
		typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
	std::unique_ptr<T> Get(const RocksDBTable& table, const rocksdb::Slice& key, const EProtocolVersion protocol = EProtocolVersion::V1) const
	{
		rocksdb::Status status;
		std::string itemStr;
		if (m_pTransaction != nullptr)
		{
			status = m_pTransaction->Get(rocksdb::ReadOptions(), table.GetHandle(), key, &itemStr);
		}
		else
		{
			status = m_pTransactionDB->GetBaseDB()->Get(rocksdb::ReadOptions(), table.GetHandle(), key, &itemStr);
		}

		if (status.ok())
		{
			std::vector<unsigned char> data(itemStr.data(), itemStr.data() + itemStr.size());
			ByteBuffer byteBuffer(std::move(data), protocol);
			return std::make_unique<T>(T::Deserialize(byteBuffer));
		}
		else if (status.IsNotFound())
		{
			//LOG_TRACE_F("Item not found with key {} in table {}", key.ToString(true), table);
			return nullptr;
		}
		else
		{
			const std::string errorMessage = StringUtil::Format(
				"Error while attempting to retrieve {} from table {}. Error: {}",
				key.ToString(true),
				table,
				status.getState()
			);
			LOG_ERROR(errorMessage);
			throw DATABASE_EXCEPTION(errorMessage);
		}
	}

	template<typename T,
		typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
	std::unique_ptr<T> Get(const std::string& tableName, const rocksdb::Slice& key, const EProtocolVersion protocol = EProtocolVersion::V1) const
	{
		return Get<T>(GetTable(tableName), key, protocol);
	}

	template<typename T,
		typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
	void Put(const RocksDBTable& table, const DBEntry<T>& entry)
	{
		rocksdb::Status status;
		std::vector<unsigned char> serialized = entry.SerializeValue();
		rocksdb::Slice value((const char*)serialized.data(), serialized.size());
		if (m_pTransaction != nullptr)
		{
			status = m_pTransaction->Put(table.GetHandle(), entry.key, value);
		}
		else
		{
			status = m_pTransactionDB->Put(rocksdb::WriteOptions(), table.GetHandle(), entry.key, value);
		}

		if (status.ok())
		{
			LOG_TRACE_F("Item inserted into table {} with key {}", table, entry.key.ToString(true));
		}
		else
		{
			const std::string errorMessage = StringUtil::Format(
				"Error while attempting to insert {} into table {}. Error: {}",
				entry.key.ToString(true),
				table,
				status.getState()
			);
			LOG_ERROR(errorMessage);
			throw DATABASE_EXCEPTION(errorMessage);
		}
	}

	template<typename T,
		typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
	void Put(const std::string& tableName, const DBEntry<T>& entry)
	{
		Put(GetTable(tableName), entry);
	}
	
    template<typename T,
        typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
    void Put(const RocksDBTable& table, const std::vector<DBEntry<T>>& entries)
	{
		assert(!entries.empty());

		std::shared_ptr<rocksdb::Transaction> pTempTransaction = nullptr;
		if (m_pTransaction == nullptr)
		{
			pTempTransaction = std::shared_ptr<rocksdb::Transaction>(m_pTransactionDB->BeginTransaction(rocksdb::WriteOptions()));
		}

		rocksdb::Status status;

		for (const DBEntry<T>& entry : entries)
		{
			std::vector<unsigned char> serialized = entry.SerializeValue();
			rocksdb::Slice value((const char*)serialized.data(), serialized.size());
			if (m_pTransaction != nullptr)
			{
				status = m_pTransaction->Put(table.GetHandle(), entry.key, value);
			}
			else
			{
				status = pTempTransaction->Put(table.GetHandle(), entry.key, value);
			}

			if (!status.ok())
			{
				const std::string errorMessage = StringUtil::Format(
					"Error while attempting to insert {} into table {}. Error: {}",
					entry.key.ToString(true),
					table,
					status.getState()
				);
				LOG_ERROR(errorMessage);
				throw DATABASE_EXCEPTION(errorMessage);
			}
		}

		if (pTempTransaction != nullptr) {
			pTempTransaction->Commit();
		}
	}

	template<typename T,
		typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
	void Put(const std::string& tableName, const std::vector<DBEntry<T>>& entries)
	{
		Put(GetTable(tableName), entries);
	}

	void Delete(const RocksDBTable& table, const rocksdb::Slice& key)
	{
		LOG_TRACE_F("Deleting {} from table {}", key.ToString(true), table);

		rocksdb::Status status;
		if (m_pTransaction != nullptr)
		{
			status = m_pTransaction->Delete(table.GetHandle(), key);
		}
		else
		{
			status = m_pTransactionDB->GetBaseDB()->Delete(rocksdb::WriteOptions(), table.GetHandle(), key);
		}

		if (!status.ok())
		{
			const std::string errorMessage = StringUtil::Format(
				"Error while attempting to delete {} from table {}. Error: {}",
				key.ToString(true),
				table,
				status.getState()
			);
			LOG_ERROR(errorMessage);
			throw DATABASE_EXCEPTION(errorMessage);
		}
	}

	void Delete(const std::string& tableName, const rocksdb::Slice& key)
	{
		Delete(GetTable(tableName), key);
	}

	void Delete(const std::string& tableName, const std::vector<std::string>& keys)
	{
		// FUTURE: Use a WriteBatch
		const RocksDBTable& table = GetTable(tableName);
		for (const rocksdb::Slice& key : keys)
		{
			Delete(table, key);
		}
	}

	void DeleteAll(const RocksDBTable& table)
	{
		LOG_WARNING_F("Deleting all rows from table {}", table);

		rocksdb::Status status;
		if (m_pTransaction != nullptr)
		{
			std::unique_ptr<rocksdb::Iterator> it(m_pTransaction->GetIterator(rocksdb::ReadOptions(), table.GetHandle()));
			for (it->SeekToFirst(); it->Valid(); it->Next())
			{
				status = m_pTransaction->Delete(table.GetHandle(), it->key());
				if (!status.ok())
				{
					const std::string errorMessage = StringUtil::Format(
						"Error while attempting to delete {} from table {}. Error: {}",
						it->key().data(),
						table,
						status.getState()
					);
					LOG_ERROR(errorMessage);
					throw DATABASE_EXCEPTION(errorMessage);
				}
			}
		}
		else
		{
			std::unique_ptr<rocksdb::Iterator> it(m_pTransactionDB->NewIterator(rocksdb::ReadOptions(), table.GetHandle()));
			for (it->SeekToFirst(); it->Valid(); it->Next())
			{
				status = m_pTransactionDB->Delete(rocksdb::WriteOptions(), table.GetHandle(), it->key());
				if (!status.ok())
				{
					const std::string errorMessage = StringUtil::Format(
						"Error while attempting to delete {} from table {}. Error: {}",
						it->key().data(),
						table,
						status.getState()
					);
					LOG_ERROR(errorMessage);
					throw DATABASE_EXCEPTION(errorMessage);
				}
			}
		}
	}

	void DeleteAll(const std::string& tableName)
	{
		DeleteAll(GetTable(tableName));
	}

	void Commit() final
	{
		//assert(m_pTransaction != nullptr);
		if (m_pTransaction != nullptr) {
			const rocksdb::Status status = m_pTransaction->Commit();
			if (!status.ok()) {
				LOG_ERROR_F("Transaction::Commit failed with error {}", status.getState());
				throw DATABASE_EXCEPTION_F("Transaction::Commit Failed with error {}", status.getState());
			}
		}
	}

	void Rollback() noexcept final
	{
		assert(m_pTransaction != nullptr);

		const rocksdb::Status status = m_pTransaction->Rollback();
		if (!status.ok())
		{
			LOG_ERROR_F("Transaction::Rollback failed with error ({})", status.getState());
		}
	}

	void OnInitWrite(const bool batch) final
	{
		if (batch) {
			m_pTransaction = std::unique_ptr<rocksdb::Transaction>(
				m_pTransactionDB->BeginTransaction(rocksdb::WriteOptions())
			);
		}
	}

	void OnEndWrite() final
	{
		m_pTransaction.reset();
	}

private:
	const RocksDBTable& GetTable(const std::string& name) const
	{
		for (const RocksDBTable& table : m_tables)
		{
			if (table.GetName() == name)
			{
				return table;
			}
		}

		LOG_ERROR_F("Database table {} not found", name);
		throw DATABASE_EXCEPTION_F("Database table {} not found", name);
	}

	std::shared_ptr<rocksdb::OptimisticTransactionDB> m_pTransactionDB;
	std::vector<RocksDBTable> m_tables;

	std::unique_ptr<rocksdb::Transaction> m_pTransaction;
};