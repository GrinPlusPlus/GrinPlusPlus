#pragma once

#include <Crypto/Models/Hash.h>
#include <P2P/Peer.h>
#include <TxPool/PoolType.h>
#include <Core/Models/Transaction.h>
#include <Common/ConcurrentQueue.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <deque>

// Forward Declarations
class Config;
class Connection;
class ConnectionManager;
class IBlockChain;
class TxHashSetArchiveMessage;

class TransactionPipe
{
public:
	static std::shared_ptr<TransactionPipe> Create(
		const Config& config,
		const std::shared_ptr<ConnectionManager>& pConnectionManager,
		const std::shared_ptr<IBlockChain>& pBlockChain
	);
	~TransactionPipe();

	bool AddTransactionToProcess(Connection& connection, const TransactionPtr& pTransaction, const EPoolType poolType);

private:
	TransactionPipe(
		const Config& config,
		const std::shared_ptr<ConnectionManager>& pConnectionManager,
		const std::shared_ptr<IBlockChain>& pBlockChain
	);

	const Config& m_config;
	std::shared_ptr<ConnectionManager> m_pConnectionManager;
	std::shared_ptr<IBlockChain> m_pBlockChain;

	static void Thread_ProcessTransactions(TransactionPipe& pipeline);
	std::thread m_transactionThread;
	struct TxEntry
	{
		TxEntry(const uint64_t connectionId, PeerPtr pPeer, TransactionPtr txn, const EPoolType type)
			: m_connectionId(connectionId), m_peer(pPeer), pTransaction(txn), poolType(type)
		{

		}

		uint64_t m_connectionId;
		PeerPtr m_peer;
		TransactionPtr pTransaction;
		EPoolType poolType;
	};

	ConcurrentQueue<TxEntry> m_transactionsToProcess;

	std::atomic_bool m_terminate;
};