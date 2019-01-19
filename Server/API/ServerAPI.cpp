#include "ServerAPI.h"
#include "../RestUtil.h"
#include "../ServerContainer.h"
#include "../RestUtil.h"

#include <P2P/Common.h>
#include <json/json.h>

int ServerAPI::GetServer_Handler(struct mg_connection* conn, void* pServerContainer)
{
	ServerContainer* pServer = (ServerContainer*)pServerContainer;
	
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
	tipNode["last_block_pushed"] = HexUtil::ConvertToHex(pTip->GetHash().GetData(), false, false);
	tipNode["prev_block_to_last"] = HexUtil::ConvertToHex(pTip->GetPreviousBlockHash().GetData(), false, false);
	tipNode["total_difficulty"] = pTip->GetTotalDifficulty();
	statusNode["tip"] = tipNode;

	return RestUtil::BuildSuccessResponse(conn, statusNode.toStyledString());
}