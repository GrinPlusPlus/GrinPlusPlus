#include "ServerAPI.h"
#include "../NodeContext.h"

#include <Net/Util/HTTPUtil.h>
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
int ServerAPI::V1_Handler(struct mg_connection* conn, void*)
{
	if (HTTPUtil::GetHTTPMethod(conn) == HTTP::EHTTPMethod::GET)
	{
		Json::Value json;
		json.append("GET /v1/blocks/<hash>?compact");
		json.append("GET /v1/blocks/<height>?compact");
		json.append("GET /v1/blocks/<output commit>?compact");
		json.append("GET /v1/chain/");
		json.append("GET /v1/chain/outputs/byids?id=xxx,yyy&id=zzz");
		json.append("GET /v1/chain/outputs/byheight?start_height=100&end_height=200");
		json.append("GET /v1/peers/all");
		json.append("GET /v1/peers/connected");
		json.append("GET /v1/peers/a.b.c.d");
		json.append("POST /v1/peers/ban?a.b.c.d");
		json.append("POST /v1/peers/unban?a.b.c.d");
		json.append("GET /v1/txhashset/roots");
		json.append("GET /v1/txhashset/lastkernels?n=###");
		json.append("GET /v1/txhashset/lastoutputs?n=###");
		json.append("GET /v1/txhashset/lastrangeproofs?n=###");
		json.append("GET /v1/txhashset/outputs?start_index=1&max=100");

		return HTTPUtil::BuildSuccessResponse(conn, json.toStyledString());
	}
	else
	{
		return HTTPUtil::BuildNotFoundResponse(conn, "Not Found");
	}
}

int ServerAPI::GetStatus_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;
	
	auto pTip = pServer->m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTip == nullptr)
	{
		return HTTPUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
	}

	Json::Value statusNode;
	statusNode["protocol_version"] = P2P::PROTOCOL_VERSION;
	statusNode["user_agent"] = P2P::USER_AGENT;

	SyncStatusConstPtr pSyncStatus = pServer->m_pP2PServer->GetSyncStatus();
	statusNode["sync_status"] = GetStatusString(*pSyncStatus);

	Json::Value stateNode;
	stateNode["downloaded"] = pSyncStatus->GetDownloaded();
	stateNode["download_size"] = pSyncStatus->GetDownloadSize();
	stateNode["processing_status"] = pSyncStatus->GetProcessingStatus();
	statusNode["state"] = stateNode;

	Json::Value networkNode;
	networkNode["height"] = pSyncStatus->GetNetworkHeight();
	networkNode["total_difficulty"] = pSyncStatus->GetNetworkDifficulty();
	const std::pair<size_t, size_t> numConnections = pServer->m_pP2PServer->GetNumberOfConnectedPeers();
	networkNode["num_inbound"] = Json::UInt64(numConnections.first);
	networkNode["num_outbound"] = Json::UInt64(numConnections.second);
	statusNode["network"] = networkNode;

	Json::Value tipNode;
	tipNode["height"] = pTip->GetHeight();
	tipNode["hash"] = pTip->GetHash().ToHex();
	tipNode["previous_hash"] = pTip->GetPreviousHash().ToHex();
	tipNode["total_difficulty"] = pTip->GetTotalDifficulty();
	statusNode["chain"] = tipNode;

	const uint64_t headerHeight = pServer->m_pBlockChain->GetHeight(EChainType::CANDIDATE);
	statusNode["header_height"] = headerHeight;

	return HTTPUtil::BuildSuccessResponse(conn, statusNode.toStyledString());
}

std::string ServerAPI::GetStatusString(const SyncStatus& syncStatus)
{
	const ESyncStatus status = syncStatus.GetStatus();
	switch (status)
	{
		case ESyncStatus::SYNCING_HEADERS:
		{
			return "SYNCING_HEADERS";
		}
		case ESyncStatus::SYNCING_TXHASHSET:
		case ESyncStatus::TXHASHSET_SYNC_FAILED:
		{
			return "DOWNLOADING_TXHASHSET";
		}
		case ESyncStatus::PROCESSING_TXHASHSET:
		{
			return "PROCESSING_TXHASHSET";
		}
		case ESyncStatus::SYNCING_BLOCKS:
		{
			return "SYNCING_BLOCKS";
		}
		case ESyncStatus::WAITING_FOR_PEERS:
		{
			return "NOT_CONNECTED";
		}
		case ESyncStatus::NOT_SYNCING:
		{
			return "FULLY_SYNCED";
		}
	}

	return "UNKNOWN";
}

int ServerAPI::ResyncChain_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	pServer->m_pBlockChain->ResyncChain();
	pServer->m_pP2PServer->UnbanAllPeers();

	return HTTPUtil::BuildSuccessResponse(conn, "");
}