#include "NodeRestServer.h"
#include "NodeContext.h"

#include "API/HeaderAPI.h"
#include "API/BlockAPI.h"
#include "API/ServerAPI.h"
#include "API/ChainAPI.h"
#include "API/PeersAPI.h"
#include "API/TxHashSetAPI.h"

#include <Common/Logger.h>
#include <Core/Global.h>
#include <Net/Util/HTTPUtil.h>

static int Shutdown_Handler(struct mg_connection* conn, void*)
{
	Global::Shutdown();
	return HTTPUtil::BuildSuccessResponse(conn, "");
}

NodeRestServer::UPtr NodeRestServer::Create(const Config& config, std::shared_ptr<NodeContext> pNodeContext)
{
	const uint16_t port = config.GetServerConfig().GetRestAPIPort();
	ServerPtr pServer = Server::Create(EServerType::LOCAL, std::make_optional<uint16_t>(port));
	NodeServer::UPtr pV2Server = NodeServer::Create(pServer, pNodeContext->m_pBlockChain, pNodeContext->m_pP2PServer);

	/* Add v1 handlers */
	pServer->AddListener("/v1/status", ServerAPI::GetStatus_Handler, pNodeContext.get());
	pServer->AddListener("/v1/resync", ServerAPI::ResyncChain_Handler, pNodeContext.get());
	pServer->AddListener("/v1/headers/", HeaderAPI::GetHeader_Handler, pNodeContext.get());
	pServer->AddListener("/v1/blocks/", BlockAPI::GetBlock_Handler, pNodeContext.get());
	pServer->AddListener("/v1/chain/outputs/byids", ChainAPI::GetChainOutputsByIds_Handler, pNodeContext.get());
	pServer->AddListener("/v1/chain/outputs/byheight", ChainAPI::GetChainOutputsByHeight_Handler, pNodeContext.get());
	pServer->AddListener("/v1/chain", ChainAPI::GetChain_Handler, pNodeContext.get());
	pServer->AddListener("/v1/peers/all", PeersAPI::GetAllPeers_Handler, pNodeContext.get());
	pServer->AddListener("/v1/peers/connected", PeersAPI::GetConnectedPeers_Handler, pNodeContext.get());
	pServer->AddListener("/v1/peers/", PeersAPI::Peer_Handler, pNodeContext.get());
	pServer->AddListener("/v1/txhashset/roots", TxHashSetAPI::GetRoots_Handler, pNodeContext.get());
	pServer->AddListener("/v1/txhashset/lastkernels", TxHashSetAPI::GetLastKernels_Handler, pNodeContext.get());
	pServer->AddListener("/v1/txhashset/lastoutputs", TxHashSetAPI::GetLastOutputs_Handler, pNodeContext.get());
	pServer->AddListener("/v1/txhashset/lastrangeproofs", TxHashSetAPI::GetLastRangeproofs_Handler, pNodeContext.get());
	pServer->AddListener("/v1/txhashset/outputs", TxHashSetAPI::GetOutputs_Handler, pNodeContext.get());
	pServer->AddListener("/v1/shutdown", Shutdown_Handler, pNodeContext.get());
	pServer->AddListener("/v1/", ServerAPI::V1_Handler, pNodeContext.get());

	return std::make_unique<NodeRestServer>(pNodeContext, std::move(pV2Server));
}