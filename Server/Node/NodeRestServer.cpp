#include "NodeRestServer.h"
#include "NodeContext.h"
#include "../ShutdownManager.h"
#include "../civetweb/include/civetweb.h"
#include "../RestUtil.h"

#include "API/HeaderAPI.h"
#include "API/BlockAPI.h"
#include "API/ServerAPI.h"
#include "API/ChainAPI.h"
#include "API/PeersAPI.h"
#include "API/TxHashSetAPI.h"

#include "API/Explorer/BlockInfoAPI.h"

#include <P2P/P2PServer.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>

NodeRestServer::NodeRestServer(const Config& config, IDatabase* pDatabase, TxHashSetManager* pTxHashSetManager, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer)
	: m_config(config), m_pDatabase(pDatabase), m_pTxHashSetManager(pTxHashSetManager), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer)
{
	m_pNodeContext = new NodeContext(pDatabase, pBlockChainServer, pP2PServer, pTxHashSetManager);
}

NodeRestServer::~NodeRestServer()
{
	delete m_pNodeContext;
}

static int Shutdown_Handler(struct mg_connection* conn, void* pNodeContext)
{
	ShutdownManager::GetInstance().Shutdown();
	return RestUtil::BuildSuccessResponse(conn, "");
}

bool NodeRestServer::Initialize()
{
	/* Start the server */
	const uint32_t port = m_config.GetServerConfig().GetRestAPIPort();
	const char* mg_options[] = {
		"num_threads", "5",
		"listening_ports", std::to_string(port).c_str(),
		NULL
	};
	m_pNodeCivetContext = mg_start(NULL, 0, mg_options);

	/* Add handlers */
	mg_set_request_handler(m_pNodeCivetContext, "/v1/explorer/blockinfo/", BlockInfoAPI::GetBlockInfo_Handler, m_pNodeContext);

	mg_set_request_handler(m_pNodeCivetContext, "/v1/status", ServerAPI::GetStatus_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/resync", ServerAPI::ResyncChain_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/headers/", HeaderAPI::GetHeader_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/blocks/", BlockAPI::GetBlock_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/chain/outputs/byids", ChainAPI::GetChainOutputsByIds_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/chain/outputs/byheight", ChainAPI::GetChainOutputsByHeight_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/chain", ChainAPI::GetChain_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/peers/all", PeersAPI::GetAllPeers_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/peers/connected", PeersAPI::GetConnectedPeers_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/peers/", PeersAPI::Peer_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/roots", TxHashSetAPI::GetRoots_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/lastkernels", TxHashSetAPI::GetLastKernels_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/lastoutputs", TxHashSetAPI::GetLastOutputs_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/lastrangeproofs", TxHashSetAPI::GetLastRangeproofs_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/outputs", TxHashSetAPI::GetOutputs_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/shutdown", Shutdown_Handler, m_pNodeContext);
	mg_set_request_handler(m_pNodeCivetContext, "/v1/", ServerAPI::V1_Handler, m_pNodeContext);

	/*
	// TODO: Still missing
		"get pool",
		"post pool/push"
		"post chain/compact"
		"post chain/validate"
	*/

	return true;
}

bool NodeRestServer::Shutdown()
{
	/* Stop the server */
	mg_stop(m_pNodeCivetContext);

	return true;
}