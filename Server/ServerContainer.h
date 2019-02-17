#pragma once

#include <Database/Database.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/P2PServer.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>

struct ServerContainer
{
	IDatabase* m_pDatabase;
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;
	TxHashSetManager* m_pTxHashSetManager;
};