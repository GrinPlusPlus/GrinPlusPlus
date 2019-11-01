#pragma once

#include "BlockPipe.h"
#include "TransactionPipe.h"
#include "TxHashSetPipe.h"

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
	Pipeline(const Config& config, ConnectionManager& connectionManager, IBlockChainServerPtr pBlockChainServer);

	void Start();
	void Stop();

	BlockPipe& GetBlockPipe() { return m_blockPipe; }
	TransactionPipe& GetTransactionPipe() { return m_transactionPipe; }
	TxHashSetPipe& GetTxHashSetPipe() { return m_txHashSetPipe; }

private:
	std::atomic_bool m_terminate;

	BlockPipe m_blockPipe;
	TransactionPipe m_transactionPipe;
	TxHashSetPipe m_txHashSetPipe;
};