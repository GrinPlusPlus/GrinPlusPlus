#pragma once

#include "../ConnectionManager.h"

#include <Crypto/Hash.h>
#include <Net/Socket.h>
#include <TxPool/PoolType.h>
#include <Core/Models/Transaction.h>
#include <BlockChain/BlockChainServer.h>
#include <Common/ConcurrentQueue.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <deque>

// Forward Declarations
class Config;
class TxHashSetArchiveMessage;

class TransactionPipe
{
public:
	static std::shared_ptr<TransactionPipe> Create(
		const Config& config,
		ConnectionManagerPtr pConnectionManager,
		IBlockChainServerPtr pBlockChainServer
	);
	~TransactionPipe();

	bool AddTransactionToProcess(const uint64_t connectionId, TransactionPtr pTransaction, const EPoolType poolType);

private:
	TransactionPipe(const Config& config, ConnectionManagerPtr pConnectionManager, IBlockChainServerPtr pBlockChainServer);

	const Config& m_config;
	ConnectionManagerPtr m_pConnectionManager;
	IBlockChainServerPtr m_pBlockChainServer;

	static void Thread_ProcessTransactions(TransactionPipe& pipeline);
	std::thread m_transactionThread;
	struct TxEntry
	{
		TxEntry(const uint64_t connId, TransactionPtr txn, const EPoolType type)
			: connectionId(connId), pTransaction(txn), poolType(type)
		{

		}

		uint64_t connectionId;
		TransactionPtr pTransaction;
		EPoolType poolType;
	};

	ConcurrentQueue<TxEntry> m_transactionsToProcess;

	std::atomic_bool m_terminate;
};