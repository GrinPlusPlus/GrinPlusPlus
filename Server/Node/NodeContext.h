#pragma once

#include <Database/Database.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/P2PServer.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>

struct NodeContext
{
	NodeContext(IDatabase* pDatabase, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer, TxHashSetManager* pTxHashSetManager)
		: m_pDatabase(pDatabase), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer), m_pTxHashSetManager(pTxHashSetManager)
	{

	}

	IDatabase* m_pDatabase;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;
	TxHashSetManager* m_pTxHashSetManager;
};