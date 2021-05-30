#include "ChainAPI.h"
#include "../NodeContext.h"

#include <Net/Util/HTTPUtil.h>
#include <Common/Util/StringUtil.h>
#include <Crypto/Crypto.h>
#include <json/json.h>
#include <BlockChain/BlockChain.h>
#include <Database/BlockDb.h>

int ChainAPI::GetChain_Handler(struct mg_connection* conn, void* pNodeContext)
{
	IBlockChain::Ptr pBlockChain = ((NodeContext*)pNodeContext)->m_pBlockChain;

	try
	{
		auto pTip = pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTip != nullptr)
		{
			Json::Value chainNode;
			chainNode["height"] = pTip->GetHeight();
			chainNode["last_block_pushed"] = pTip->GetHash().ToHex();
			chainNode["prev_block_to_last"] = pTip->GetPreviousHash().ToHex();
			chainNode["total_difficulty"] = pTip->GetTotalDifficulty();

			return HTTPUtil::BuildSuccessResponse(conn, chainNode.toStyledString());
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	return HTTPUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
}

int ChainAPI::GetChainOutputsByHeight_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	try
	{
		uint64_t startHeight = 0;
		uint64_t endHeight = 0;
		const std::string queryString = HTTPUtil::GetQueryString(conn);
		if (!queryString.empty())
		{
			std::vector<std::string> tokens = StringUtil::Split(queryString, "&");
			for (const std::string& token : tokens)
			{
				if (StringUtil::StartsWith(token, "start_height="))
				{
					std::vector<std::string> startHeightTokens = StringUtil::Split(token, "=");
					if (startHeightTokens.size() != 2)
					{
						return HTTPUtil::BuildBadRequestResponse(conn, "Expected /v1/chain/outputs/byheight?start_height=1&end_height=100");
					}

					startHeight = std::stoull(startHeightTokens[1]);
				}
				else if (StringUtil::StartsWith(token, "end_height="))
				{
					std::vector<std::string> endHeightTokens = StringUtil::Split(token, "=");
					if (endHeightTokens.size() != 2)
					{
						return HTTPUtil::BuildBadRequestResponse(conn, "Expected /v1/chain/outputs/byheight?start_height=1&end_height=100");
					}

					endHeight = std::stoull(endHeightTokens[1]);
				}
			}
		}

		if (endHeight == 0 || endHeight < startHeight)
		{
			endHeight = startHeight;
		}

		Json::Value json;

		std::vector<BlockWithOutputs> blocksWithOutputs = pServer->m_pBlockChain->GetOutputsByHeight(startHeight, endHeight);
		for (const BlockWithOutputs& block : blocksWithOutputs)
		{
			/*

			"header": {
			  "hash": "40adad0aec27797b48840aa9e00472015c21baea118ce7a2ff1a82c0f8f5bf82",
			  "height": 0,
			  "previous": "0000000000000000000000000000000000000000000000000000000000000000"
			},
			"outputs": [
			  {
				"output_type": "Coinbase",
				"commit": "08b7e57c448db5ef25aa119dde2312c64d7ff1b890c416c6dda5ec73cbfed2edea",
				"spent": false,
				"proof": null,
				"proof_hash": "6c301688d9186c3a99444f827bdfe3b858fe87fc314737a4dc1155d9884491d2",
				"block_height": 0,
				"merkle_proof": "00000000000000010000000000000000",
				"mmr_index": 1
			  }
			]
			*/
			Json::Value blockJSON;
			blockJSON["header"] = block.GetBlockIdentifier().ToJSON();

			Json::Value outputsJSON;
			for (const OutputDTO& output : block.GetOutputs())
			{
				outputsJSON.append(output.ToJSON());
			}

			blockJSON["outputs"] = outputsJSON;

			json.append(blockJSON);
		}

		return HTTPUtil::BuildSuccessResponse(conn, json.toStyledString());
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Excpetion thrown: {}", e.what());
		return HTTPUtil::BuildBadRequestResponse(conn, "Expected /v1/chain/outputs/byheight?start_height=1&end_height=100");
	}
}

int ChainAPI::GetChainOutputsByIds_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	try
	{
		std::vector<std::string> ids;
		const std::string queryString = HTTPUtil::GetQueryString(conn);
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
						return HTTPUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/outputs?start_index=1&max=100");
					}

					std::vector<std::string> tempIds = StringUtil::Split(startIndexTokens[1], ",");
					ids.insert(ids.end(), tempIds.begin(), tempIds.end());
				}
			}
		}

		std::shared_ptr<Locked<IBlockDB>> pBlockDB = pServer->m_pDatabase->GetBlockDB();

		Json::Value rootNode;
		for (const std::string& id : ids)
		{
			Commitment commitment = Commitment::FromHex(id);
			std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->Read()->GetOutputPosition(commitment);
			if (pOutputPosition != nullptr)
			{
				Json::Value outputNode;
				outputNode["commit"] = commitment.Format();
				outputNode["height"] = pOutputPosition->GetBlockHeight();
				outputNode["mmr_index"] = pOutputPosition->GetPosition() + 1;

				rootNode.append(outputNode);
			}
		}

		return HTTPUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		return HTTPUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/outputs?start_index=1&max=100");
	}
}