#pragma once

#include <BlockChainServer.h>
#include <P2PServer.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>

struct ServerContainer
{
	IBlockChainServer* m_pBlockChainServer;
	IP2PServer* m_pP2PServer;
	TxHashSetManager* m_pTxHashSetManager;
};