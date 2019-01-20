#include "ChainAPI.h"
#include "../RestUtil.h"

#include <HexUtil.h>
#include <json/json.h>
#include <BlockChainServer.h>

int ChainAPI::GetChain_Handler(struct mg_connection* conn, void* pBlockChainServer)
{
	IBlockChainServer* pBlockChain = (IBlockChainServer*)pBlockChainServer;
	
	std::unique_ptr<BlockHeader> pTip = pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTip != nullptr)
	{
		Json::Value chainNode;
		chainNode["height"] = pTip->GetHeight();
		chainNode["last_block_pushed"] = HexUtil::ConvertToHex(pTip->GetHash().GetData(), false, false);
		chainNode["prev_block_to_last"] = HexUtil::ConvertToHex(pTip->GetPreviousBlockHash().GetData(), false, false);
		chainNode["total_difficulty"] = pTip->GetTotalDifficulty();

		return RestUtil::BuildSuccessResponse(conn, chainNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
	}
}