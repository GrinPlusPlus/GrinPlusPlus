#pragma once

#include <Database/Database.h>
#include <BlockChain/BlockChain.h>
#include <P2P/P2PServer.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

struct NodeContext
{
	IDatabasePtr m_pDatabase;
	IBlockChain::Ptr m_pBlockChain;
	IP2PServerPtr m_pP2PServer;
	TxHashSetManager::Ptr m_pTxHashSetManager;
	ITransactionPool::Ptr m_pTransactionPool;
};