#include "ChainAPI.h"
#include "../../RestUtil.h"
#include "../NodeContext.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Crypto/Crypto.h>
#include <json/json.h>
#include <BlockChain/BlockChainServer.h>

int ChainAPI::GetChain_Handler(struct mg_connection* conn, void* pNodeContext)
{
	IBlockChainServer* pBlockChainServer = ((NodeContext*)pNodeContext)->m_pBlockChainServer;
	
	std::unique_ptr<BlockHeader> pTip = pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTip != nullptr)
	{
		Json::Value chainNode;
		chainNode["height"] = pTip->GetHeight();
		chainNode["last_block_pushed"] = HexUtil::ConvertToHex(pTip->GetHash().GetData());
		chainNode["prev_block_to_last"] = HexUtil::ConvertToHex(pTip->GetPreviousBlockHash().GetData());
		chainNode["total_difficulty"] = pTip->GetTotalDifficulty();

		return RestUtil::BuildSuccessResponse(conn, chainNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
	}
}

int ChainAPI::GetChainOutputsByHeight_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	uint64_t startHeight = 0;
	uint64_t endHeight = 0;
	const std::string queryString = RestUtil::GetQueryString(conn);
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
					return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/chain/outputs/byheight?start_height=1&end_height=100");
				}

				startHeight = std::stoull(startHeightTokens[1]);
			}
			else if (StringUtil::StartsWith(token, "end_height="))
			{
				std::vector<std::string> endHeightTokens = StringUtil::Split(token, "=");
				if (endHeightTokens.size() != 2)
				{
					return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/chain/outputs/byheight?start_height=1&end_height=100");
				}

				endHeight = std::stoull(endHeightTokens[1]);
			}
		}
	}

	if (endHeight == 0 || endHeight < startHeight)
	{
		endHeight = startHeight;
	}

	Json::Value rootNode;

	std::vector<BlockWithOutputs> blocksWithOutputs = pServer->m_pBlockChainServer->GetOutputsByHeight(startHeight, endHeight);
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
		Json::Value blockNode;

		Json::Value headerNode;
		headerNode["hash"] = HexUtil::ConvertToHex(block.GetBlockIdentifier().GetHash().GetData());
		headerNode["height"] = block.GetBlockIdentifier().GetHeight();
		headerNode["previous"] = HexUtil::ConvertToHex(block.GetBlockIdentifier().GetPreviousHash().GetData());
		blockNode["header"] = headerNode;

		Json::Value outputsNode;
		for (const OutputDisplayInfo& output : block.GetOutputs())
		{
			// TODO: Move to JSONFactory
			Json::Value outputNode;
			const EOutputFeatures features = output.GetIdentifier().GetFeatures();
			outputNode["output_type"] = features == DEFAULT_OUTPUT ? "Transaction" : "Coinbase";
			outputNode["commit"] = output.GetIdentifier().GetCommitment().ToHex();
			outputNode["spent"] = false;
			outputNode["proof"] = HexUtil::ConvertToHex(output.GetRangeProof().GetProofBytes());

			Serializer proofSerializer;
			output.GetRangeProof().Serialize(proofSerializer);
			outputNode["proof_hash"] = HexUtil::ConvertToHex(Crypto::Blake2b(proofSerializer.GetBytes()).GetData());

			outputNode["block_height"] = output.GetLocation().GetBlockHeight();
			outputNode["merkle_proof"] = Json::nullValue;
			outputNode["mmr_index"] = output.GetLocation().GetMMRIndex() + 1;
			outputsNode.append(outputNode);
		}

		blockNode["outputs"] = outputsNode;

		rootNode.append(blockNode);
	}

	return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
}

int ChainAPI::GetChainOutputsByIds_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

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
			outputNode["commit"] = HexUtil::ConvertToHex(commitment.GetCommitmentBytes().GetData());
			outputNode["height"] = locationOpt.value().GetBlockHeight();
			outputNode["mmr_index"] = locationOpt.value().GetMMRIndex() + 1;

			rootNode.append(outputNode);
		}
	}

	return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
}