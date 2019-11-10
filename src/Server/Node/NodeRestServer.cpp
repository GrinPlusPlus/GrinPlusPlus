#include "NodeRestServer.h"
#include "NodeContext.h"
#include <civetweb.h>

#include "API/HeaderAPI.h"
#include "API/BlockAPI.h"
#include "API/ServerAPI.h"
#include "API/ChainAPI.h"
#include "API/PeersAPI.h"
#include "API/TxHashSetAPI.h"

#include "API/Explorer/BlockInfoAPI.h"

#include <Infrastructure/Logger.h>
#include <Infrastructure/ShutdownManager.h>
#include <Net/Util/HTTPUtil.h>
#include <P2P/P2PServer.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>

NodeRestServer::NodeRestServer(const Config& config, std::shared_ptr<NodeContext> pNodeContext)
	: m_config(config), m_pNodeContext(pNodeContext)
{

}

NodeRestServer::~NodeRestServer()
{
	try
	{
		mg_stop(m_pNodeCivetContext);
	}
	catch (const std::system_error& e)
	{
		LOG_ERROR_F("Exception thrown while stopping node API listener: %s", e.what());
	}
}

std::shared_ptr<NodeRestServer> NodeRestServer::Create(const Config& config, std::shared_ptr<NodeContext> pNodeContext)
{
	auto pNodeRestServer = std::shared_ptr<NodeRestServer>(new NodeRestServer(config, pNodeContext));
	pNodeRestServer->Initialize();
	return pNodeRestServer;
}

static int Shutdown_Handler(struct mg_connection* conn, void* pNodeContext)
{
	ShutdownManagerAPI::Shutdown();
	return HTTPUtil::BuildSuccessResponse(conn, "");
}

void NodeRestServer::Initialize()
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
	mg_set_request_handler(m_pNodeCivetContext, "/v1/explorer/blockinfo/", BlockInfoAPI::GetBlockInfo_Handler, m_pNodeContext.get());

	mg_set_request_handler(m_pNodeCivetContext, "/v1/status", ServerAPI::GetStatus_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/resync", ServerAPI::ResyncChain_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/headers/", HeaderAPI::GetHeader_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/blocks/", BlockAPI::GetBlock_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/chain/outputs/byids", ChainAPI::GetChainOutputsByIds_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/chain/outputs/byheight", ChainAPI::GetChainOutputsByHeight_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/chain", ChainAPI::GetChain_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/peers/all", PeersAPI::GetAllPeers_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/peers/connected", PeersAPI::GetConnectedPeers_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/peers/", PeersAPI::Peer_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/roots", TxHashSetAPI::GetRoots_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/lastkernels", TxHashSetAPI::GetLastKernels_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/lastoutputs", TxHashSetAPI::GetLastOutputs_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/lastrangeproofs", TxHashSetAPI::GetLastRangeproofs_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/txhashset/outputs", TxHashSetAPI::GetOutputs_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/shutdown", Shutdown_Handler, m_pNodeContext.get());
	mg_set_request_handler(m_pNodeCivetContext, "/v1/", ServerAPI::V1_Handler, m_pNodeContext.get());
}