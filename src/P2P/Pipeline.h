#pragma once

#include <BlockChain/BlockChainServer.h>
#include <TxPool/PoolType.h>
#include <Crypto/Hash.h>
#include <deque>
#include <shared_mutex>
#include <thread>
#include <atomic>

// Forward Declarations
class ConnectionManager;

class Pipeline
{
public:
	Pipeline(const Config& config, ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	void Start();
	void Stop();

	bool AddBlockToProcess(const uint64_t connectionId, const FullBlock& block);
	bool IsProcessingBlock(const Hash& hash) const;

	bool AddTransactionToProcess(const uint64_t connectionId, const Transaction& transaction, const EPoolType poolType);
	bool IsProcessingTransaction(const Hash& hash) const;

	bool AddTxHashSetToProcess(const uint64_t connectionId, const Hash& blockHash, const std::string& path);

private:
	const Config& m_config;
	ConnectionManager& m_connectionManager;
	IBlockChainServer& m_blockChainServer;
	std::atomic<bool> m_terminate;

	// Blocks
	static void Thread_ProcessBlocks(Pipeline& pipeline);
	mutable std::shared_mutex m_blockMutex;
	std::thread m_blockThread;
	struct BlockEntry
	{
		BlockEntry(const uint64_t connId, const FullBlock& fullBlock)
			: connectionId(connId), block(fullBlock)
		{

		}

		uint64_t connectionId;
		FullBlock block;
	};
	std::deque<BlockEntry> m_blocksToProcess;

	// Transactions
	static void Thread_ProcessTransactions(Pipeline& pipeline);
	mutable std::shared_mutex m_transactionMutex;
	std::thread m_transactionThread;
	struct TxEntry
	{
		TxEntry(const uint64_t connId, const Transaction& txn, const EPoolType type)
			: connectionId(connId), transaction(txn), poolType(type)
		{

		}

		uint64_t connectionId;
		Transaction transaction;
		EPoolType poolType;
	};
	std::deque<TxEntry> m_transactionsToProcess;

	// TxHashSet
	static void Thread_ProcessTxHashSet(Pipeline& pipeline, const uint64_t connectionId, const Hash blockHash, const std::string path);
	std::thread m_txHashSetThread;
};