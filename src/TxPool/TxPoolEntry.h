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
	TxPoolEntry(TransactionPtr pTransaction, const EDandelionStatus status, const std::time_t timestamp)
		: m_pTransaction(pTransaction), m_status(status), m_timestamp(timestamp)
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
	inline bool operator<(const TxPoolEntry& txPoolEntry) const { return m_pTransaction->GetHash() < txPoolEntry.m_pTransaction->GetHash(); }
	inline bool operator==(const TxPoolEntry& txPoolEntry) const { return m_pTransaction->GetHash() == txPoolEntry.m_pTransaction->GetHash(); }
	inline bool operator!=(const TxPoolEntry& txPoolEntry) const { return m_pTransaction->GetHash() != txPoolEntry.m_pTransaction->GetHash(); }

	//
	// Getters
	//
	inline TransactionPtr GetTransaction() const { return m_pTransaction; }
	inline EDandelionStatus GetStatus() const { return m_status; }
	inline std::time_t GetTimestamp() const { return m_timestamp; }

	//
	// Setters
	//
	inline void SetStatus(const EDandelionStatus status) { m_status = status; }

private:
	TransactionPtr m_pTransaction;
	EDandelionStatus m_status;
	std::time_t m_timestamp;
};