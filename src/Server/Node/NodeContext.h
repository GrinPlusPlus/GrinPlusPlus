#pragma once

#include <Database/Database.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/P2PServer.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>

struct NodeContext
{
	NodeContext(IDatabasePtr pDatabase, IBlockChainServerPtr pBlockChainServer, IP2PServerPtr pP2PServer, TxHashSetManagerPtr pTxHashSetManager, ITransactionPoolPtr pTransactionPool)
		: m_pDatabase(pDatabase), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer), m_pTxHashSetManager(pTxHashSetManager), m_pTransactionPool(pTransactionPool)
	{

	}

	IDatabasePtr m_pDatabase;
	IBlockChainServerPtr m_pBlockChainServer;
	IP2PServerPtr m_pP2PServer;
	TxHashSetManagerPtr m_pTxHashSetManager;
	ITransactionPoolPtr m_pTransactionPool;
};