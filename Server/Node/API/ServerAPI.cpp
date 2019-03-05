#include "ServerAPI.h"
#include "../../RestUtil.h"
#include "../NodeContext.h"

#include <P2P/Common.h>
#include <json/json.h>

/*
	mg_set_request_handler(ctx, "/v1/status", ServerAPI::GetStatus_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/headers/", HeaderAPI::GetHeader_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/blocks/", BlockAPI::GetBlock_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/chain/outputs/byids", ChainAPI::GetChainOutputsByIds_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/chain/outputs/byheight", ChainAPI::GetChainOutputsByHeight_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/chain", ChainAPI::GetChain_Handler, m_pBlockChainServer);
	mg_set_request_handler(ctx, "/v1/peers/all", PeersAPI::GetAllPeers_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/peers/connected", PeersAPI::GetConnectedPeers_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/peers/", PeersAPI::Peer_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/txhashset/roots", TxHashSetAPI::GetRoots_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/txhashset/lastkernels", TxHashSetAPI::GetLastKernels_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/txhashset/lastoutputs", TxHashSetAPI::GetLastOutputs_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/txhashset/lastrangeproofs", TxHashSetAPI::GetLastRangeproofs_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/txhashset/outputs", TxHashSetAPI::GetOutputs_Handler, &m_nodeContext);
	mg_set_request_handler(ctx, "/v1/", ServerAPI::V1_Handler, &m_nodeContext);
*/
int ServerAPI::V1_Handler(struct mg_connection* conn, void* pVoid)
{
	if (RestUtil::GetHTTPMethod(conn) == EHTTPMethod::GET)
	{
		Json::Value rootNode;
		rootNode.append("GET /v1/blocks/<hash>?compact");
		rootNode.append("GET /v1/blocks/<height>?compact");
		rootNode.append("GET /v1/blocks/<output commit>?compact");
		rootNode.append("GET /v1/chain/");
		rootNode.append("GET /v1/chain/outputs/byids?id=xxx,yyy&id=zzz");
		rootNode.append("GET /v1/chain/outputs/byheight?start_height=100&end_height=200");
		rootNode.append("GET /v1/peers/all");
		rootNode.append("GET /v1/peers/connected");
		rootNode.append("GET /v1/peers/a.b.c.d");
		rootNode.append("POST /v1/peers/ban?a.b.c.d");
		rootNode.append("POST /v1/peers/unban?a.b.c.d");
		rootNode.append("GET /v1/txhashset/roots");
		rootNode.append("GET /v1/txhashset/lastkernels?n=###");
		rootNode.append("GET /v1/txhashset/lastoutputs?n=###");
		rootNode.append("GET /v1/txhashset/lastrangeproofs?n=###");
		rootNode.append("GET /v1/txhashset/outputs?start_index=1&max=100");

		return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildNotFoundResponse(conn, "Not Found");
	}
}

int ServerAPI::GetStatus_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;
	
	std::unique_ptr<BlockHeader> pTip = pServer->m_pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTip == nullptr)
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
	}

	Json::Value statusNode;
	statusNode["protocol_version"] = P2P::PROTOCOL_VERSION;
	statusNode["user_agent"] = P2P::USER_AGENT;
	statusNode["connections"] = pServer->m_pP2PServer->GetNumberOfConnectedPeers();

	Json::Value tipNode;
	tipNode["height"] = pTip->GetHeight();
	tipNode["last_block_pushed"] = HexUtil::ConvertToHex(pTip->GetHash().GetData());
	tipNode["prev_block_to_last"] = HexUtil::ConvertToHex(pTip->GetPreviousBlockHash().GetData());
	tipNode["total_difficulty"] = pTip->GetTotalDifficulty();
	statusNode["tip"] = tipNode;

	return RestUtil::BuildSuccessResponse(conn, statusNode.toStyledString());
}