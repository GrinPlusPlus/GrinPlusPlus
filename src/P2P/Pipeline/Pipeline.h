#pragma once

#include "../ConnectionManager.h"
#include "BlockPipe.h"
#include "TransactionPipe.h"
#include "TxHashSetPipe.h"

#include <P2P/SyncStatus.h>
#include <BlockChain/BlockChainServer.h>
#include <memory>

class Pipeline
{
public:
	static std::shared_ptr<Pipeline> Create(
		const Config& config,
		ConnectionManagerPtr pConnectionManager,
		IBlockChainServerPtr pBlockChainServer,
		SyncStatusPtr pSyncStatus)
	{
		std::shared_ptr<BlockPipe> pBlockPipe = BlockPipe::Create(config, pBlockChainServer);
		std::shared_ptr<TransactionPipe> pTransactionPipe = TransactionPipe::Create(config, pConnectionManager, pBlockChainServer);
		std::shared_ptr<TxHashSetPipe> pTxHashSetPipe = TxHashSetPipe::Create(config, pBlockChainServer, pSyncStatus);

		return std::shared_ptr<Pipeline>(new Pipeline(pBlockPipe, pTransactionPipe, pTxHashSetPipe));
	}

	std::shared_ptr<BlockPipe> GetBlockPipe() { return m_pBlockPipe; }
	std::shared_ptr<TransactionPipe> GetTransactionPipe() { return m_pTransactionPipe; }
	std::shared_ptr<TxHashSetPipe> GetTxHashSetPipe() { return m_pTxHashSetPipe; }

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