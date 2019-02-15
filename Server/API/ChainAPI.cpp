#include "ChainAPI.h"
#include "../RestUtil.h"
#include "../ServerContainer.h"

#include <HexUtil.h>
#include <StringUtil.h>
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

int ChainAPI::GetChainOutputsByHeight_Handler(struct mg_connection* conn, void* pServerContainer)
{
	// TODO: Implement
	const std::string response = "NOT IMPLEMENTED";
	return RestUtil::BuildBadRequestResponse(conn, response);
}

int ChainAPI::GetChainOutputsByIds_Handler(struct mg_connection* conn, void* pServerContainer)
{
	ServerContainer* pServer = (ServerContainer*)pServerContainer;

	std::vector<std::string> ids;
	const std::string queryString = RestUtil::GetQueryString(conn);
	if (!queryString.empty())
	{
		std::vector<std::string> tokens = StringUtil::Split(queryString, "&");
		for (const std::string& token : tokens)
		{
			if (StringUtil::StartsWith(token, "id="))
			{
				std::vector<std::string> startIndexTokens = StringUtil::Split(token, "=");
				if (startIndexTokens.size() != 2)
				{
					return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/outputs?start_index=1&max=100");
				}

				std::vector<std::string> tempIds = StringUtil::Split(startIndexTokens[1], ",");
				ids.insert(ids.end(), tempIds.begin(), tempIds.end());
			}
		}
	}

	IBlockDB& blockDB = pServer->m_pDatabase->GetBlockDB();

	Json::Value rootNode;
	for (const std::string& id : ids)
	{
		Commitment commitment(CBigInteger<33>::FromHex(id));
		std::optional<OutputLocation> locationOpt = blockDB.GetOutputPosition(commitment);
		if (locationOpt.has_value())
		{
			Json::Value outputNode;
			outputNode["commit"] = HexUtil::ConvertToHex(commitment.GetCommitmentBytes().GetData(), false, false);
			outputNode["height"] = locationOpt.value().GetBlockHeight();
			outputNode["mmr_index"] = locationOpt.value().GetMMRIndex() + 1;

			rootNode.append(outputNode);
		}
	}

	return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
}