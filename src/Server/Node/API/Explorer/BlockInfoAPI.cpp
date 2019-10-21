#include "BlockInfoAPI.h"
#include <JSONFactory.h>
#include <Node/NodeContext.h>

#include <Net/Util/HTTPUtil.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <json/json.h>

/*
  "get /v1/explorer/blockinfo/latest?num_blocks=100"
*/


int BlockInfoAPI::GetBlockInfo_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	HTTP::EHTTPMethod httpMethod = HTTPUtil::GetHTTPMethod(conn);
	if (httpMethod == HTTP::EHTTPMethod::GET)
	{
		const std::string uriParam = HTTPUtil::GetURIParam(conn, "/v1/explorer/blockinfo/");
		if (uriParam == "latest")
		{
			return GetLatestBlockInfo(conn, *pServer);
		}
	}

	return HTTPUtil::BuildBadRequestResponse(conn, "Endpoint not found.");
}

int BlockInfoAPI::GetLatestBlockInfo(struct mg_connection* conn, NodeContext& server)
{
	int numBlocks = 20;
	std::optional<std::string> numBlocksOpt = HTTPUtil::GetQueryParam(conn, "num_blocks");
	if (numBlocksOpt.has_value())
	{
		numBlocks = std::stoi(numBlocksOpt.value());
	}

	std::unique_ptr<BlockHeader> pTipHeader = server.m_pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTipHeader != nullptr)
	{
		const uint64_t tipHeight = pTipHeader->GetHeight();

		Json::Value rootNode;

		Json::Value blocksNode;
		numBlocks = (int)(std::min)((uint64_t)numBlocks, tipHeight + 1);
		for (int i = 0; i < numBlocks; i++)
		{
			std::unique_ptr<BlockHeader> pBlockHeader = server.m_pBlockChainServer->GetBlockHeaderByHeight(tipHeight - i, EChainType::CONFIRMED);
			if (pBlockHeader != nullptr)
			{
				Json::Value blockNode;

				blockNode["height"] = pBlockHeader->GetHeight();
				blockNode["hash"] = HexUtil::ConvertToHex(pBlockHeader->GetHash().GetData());
				blockNode["timestamp"] = pBlockHeader->GetTimestamp();
				blockNode["pow"] = (pBlockHeader->GetProofOfWork().IsPrimary() ? "AT" : "AR") + std::to_string(pBlockHeader->GetProofOfWork().GetEdgeBits());
				blockNode["total_difficulty"] = pBlockHeader->GetTotalDifficulty();

				if (pBlockHeader->GetHeight() > 0)
				{
					blockNode["difficulty"] = pBlockHeader->GetTotalDifficulty() - server.m_pBlockChainServer->GetBlockHeaderByHash(pBlockHeader->GetPreviousBlockHash())->GetTotalDifficulty();
				}
				else
				{
					blockNode["difficulty"] = pBlockHeader->GetTotalDifficulty();
				}

				// TODO: Inputs, outputs, and kernels

				// TODO: Size (bytes)? Weight?

				blocksNode.append(blockNode);
			}
		}


		rootNode["blocks"] = blocksNode;

		return HTTPUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return HTTPUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
	}
}