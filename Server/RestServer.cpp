#include "RestServer.h"
#include "civetweb/include/civetweb.h"

#include "API/HeaderAPI.h"
#include "API/BlockAPI.h"
#include "API/ServerAPI.h"
#include "API/ChainAPI.h"

#include <P2PServer.h>
#include <BlockChainServer.h>
#include <Database/Database.h>

RestServer::RestServer(const Config& config, IDatabase* pDatabase, TxHashSetManager* pTxHashSetManager, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer)
	: m_config(config), m_pDatabase(pDatabase), m_pTxHashSetManager(pTxHashSetManager), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer)
{
	m_serverContainer.m_pBlockChainServer = m_pBlockChainServer;
	m_serverContainer.m_pP2PServer = m_pP2PServer;
	m_serverContainer.m_pTxHashSetManager = m_pTxHashSetManager;
}

bool RestServer::Start()
{
	/* Initialize the library */
	mg_init_library(0);

	/* Start the server */	
	const char* mg_options[] = {
		"num_threads", "3"
	};
	ctx = mg_start(NULL, 0, NULL);// mg_options);

	/* Add handlers */
	mg_set_request_handler(ctx, "/v1/headers/", HeaderAPI::GetHeader_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/blocks/", BlockAPI::GetBlock_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/status", ServerAPI::GetServer_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/chain", ChainAPI::GetChain_Handler, m_pBlockChainServer);

	return true;
}

bool RestServer::Shutdown()
{
	/* Stop the server */
	mg_stop(ctx);

	/* Un-initialize the library */
	mg_exit_library();

	return true;
}