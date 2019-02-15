#include "NodeRestServer.h"
#include "civetweb/include/civetweb.h"

#include "API/HeaderAPI.h"
#include "API/BlockAPI.h"
#include "API/ServerAPI.h"
#include "API/ChainAPI.h"
#include "API/PeersAPI.h"
#include "API/TxHashSetAPI.h"

#include <P2PServer.h>
#include <BlockChainServer.h>
#include <Database/Database.h>

NodeRestServer::NodeRestServer(const Config& config, IDatabase* pDatabase, TxHashSetManager* pTxHashSetManager, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer)
	: m_config(config), m_pDatabase(pDatabase), m_pTxHashSetManager(pTxHashSetManager), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer)
{
	m_serverContainer.m_pBlockChainServer = m_pBlockChainServer;
	m_serverContainer.m_pP2PServer = m_pP2PServer;
	m_serverContainer.m_pTxHashSetManager = m_pTxHashSetManager;
}

bool NodeRestServer::Start()
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
	mg_set_request_handler(ctx, "/v1/peers/all", PeersAPI::GetAllPeers_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/peers/connected", PeersAPI::GetConnectedPeers_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/peers/", PeersAPI::Peer_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/roots", TxHashSetAPI::GetRoots_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/lastkernels", TxHashSetAPI::GetLastKernels_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/lastoutputs", TxHashSetAPI::GetLastOutputs_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/lastrangeproofs", TxHashSetAPI::GetLastRangeproofs_Handler, &m_serverContainer);
	mg_set_request_handler(ctx, "/v1/txhashset/outputs", TxHashSetAPI::GetOutputs_Handler, &m_serverContainer);

	/*
	// TODO: Still missing
		"get pool",
		"post pool/push"
		"post chain/compact"
		"post chain/validate"
		"get chain/outputs"
	*/

	return true;
}

bool NodeRestServer::Shutdown()
{
	/* Stop the server */
	mg_stop(ctx);

	/* Un-initialize the library */
	mg_exit_library();

	return true;
}