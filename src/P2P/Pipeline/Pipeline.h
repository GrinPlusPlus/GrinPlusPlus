#pragma once

#include "../ConnectionManager.h"
#include "../Connection.h"
#include "BlockPipe.h"
#include "TransactionPipe.h"
#include "TxHashSetPipe.h"

#include <P2P/SyncStatus.h>
#include <BlockChain/BlockChain.h>
#include <memory>

// Forward Declarations
class Connection;

class Pipeline
{
public:
	static std::shared_ptr<Pipeline> Create(
		const Config& config,
		ConnectionManagerPtr pConnectionManager,
		const IBlockChain::Ptr& pBlockChain,
		SyncStatusPtr pSyncStatus)
	{
		std::shared_ptr<BlockPipe> pBlockPipe = BlockPipe::Create(config, pBlockChain);
		std::shared_ptr<TransactionPipe> pTransactionPipe = TransactionPipe::Create(config, pConnectionManager, pBlockChain);
		std::shared_ptr<TxHashSetPipe> pTxHashSetPipe = TxHashSetPipe::Create(pConnectionManager, pBlockChain, pSyncStatus);

		return std::shared_ptr<Pipeline>(new Pipeline(pBlockPipe, pTransactionPipe, pTxHashSetPipe));
	}

	std::shared_ptr<BlockPipe> GetBlockPipe() { return m_pBlockPipe; }
	std::shared_ptr<TransactionPipe> GetTransactionPipe() { return m_pTransactionPipe; }
	std::shared_ptr<TxHashSetPipe> GetTxHashSetPipe() { return m_pTxHashSetPipe; }

	void ProcessBlock(Connection& connection, const FullBlock& block)
	{
		m_pBlockPipe->AddBlockToProcess(connection.GetPeer(), block);
	}

	void ProcessTransaction(Connection& connection, const TransactionPtr& pTransaction, const EPoolType poolType)
	{
		m_pTransactionPipe->AddTransactionToProcess(connection, pTransaction, poolType);
	}

	void ReceiveTxHashSet(const Connection::Ptr& pConnection, const TxHashSetArchiveMessage& message)
	{
		m_pTxHashSetPipe->ReceiveTxHashSet(pConnection, message);
	}

	void SendTxHashSet(const Connection::Ptr& pConnection, const Hash& block_hash)
	{
		m_pTxHashSetPipe->SendTxHashSet(pConnection, block_hash);
	}

private:
	Pipeline(
		std::shared_ptr<BlockPipe> pBlockPipe,
		std::shared_ptr<TransactionPipe> pTransactionPipe,
		std::shared_ptr<TxHashSetPipe> pTxHashSetPipe)
		: m_pBlockPipe(pBlockPipe),
		m_pTransactionPipe(pTransactionPipe),
		m_pTxHashSetPipe(pTxHashSetPipe)
	{

	}

	std::shared_ptr<BlockPipe> m_pBlockPipe;
	std::shared_ptr<TransactionPipe> m_pTransactionPipe;
	std::shared_ptr<TxHashSetPipe> m_pTxHashSetPipe;
};