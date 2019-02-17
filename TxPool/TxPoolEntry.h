#pragma once

#include <Core/Models/Transaction.h>
#include <TxPool/DandelionStatus.h>
#include <ctime>

class TxPoolEntry
{
public:
	//
	// Constructors
	//
	TxPoolEntry(const Transaction& transaction, const EDandelionStatus status, const std::time_t timestamp)
		: m_transaction(transaction), m_status(status), m_timestamp(timestamp)
	{

	}
	TxPoolEntry(Transaction&& transaction, const EDandelionStatus status, const std::time_t timestamp)
		: m_transaction(std::move(transaction)), m_status(status), m_timestamp(timestamp)
	{

	}
	TxPoolEntry(const TxPoolEntry& txPoolEntry) = default;
	TxPoolEntry(TxPoolEntry&& txPoolEntry) noexcept = default;
	TxPoolEntry() = default;

	//
	// Destructor
	//
	~TxPoolEntry() = default;

	//
	// Operators
	//
	TxPoolEntry& operator=(const TxPoolEntry& txPoolEntry) = default;
	TxPoolEntry& operator=(TxPoolEntry&& txPoolEntry) noexcept = default;
	inline bool operator<(const TxPoolEntry& txPoolEntry) const { return m_transaction.GetHash() < txPoolEntry.m_transaction.GetHash(); }
	inline bool operator==(const TxPoolEntry& txPoolEntry) const { return m_transaction.GetHash() == txPoolEntry.m_transaction.GetHash(); }
	inline bool operator!=(const TxPoolEntry& txPoolEntry) const { return m_transaction.GetHash() != txPoolEntry.m_transaction.GetHash(); }

	//
	// Getters
	//
	inline const Transaction& GetTransaction() const { return m_transaction; }
	inline EDandelionStatus GetStatus() const { return m_status; }
	inline std::time_t GetTimestamp() const { return m_timestamp; }

	//
	// Setters
	//
	inline void SetStatus(const EDandelionStatus status) { m_status = status; }

private:
	Transaction m_transaction;
	EDandelionStatus m_status;
	std::time_t m_timestamp;
};