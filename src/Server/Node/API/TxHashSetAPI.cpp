#include "TxHashSetAPI.h"
#include "../../RestUtil.h"
#include "../../JSONFactory.h"
#include "../NodeContext.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Crypto/Crypto.h>
#include <json/json.h>

/*
  "get txhashset/roots",
  "get txhashset/lastoutputs?n=10",
  "get txhashset/lastrangeproofs",
  "get txhashset/lastkernels",
  "get txhashset/outputs?start_index=1&max=100",
*/


int TxHashSetAPI::GetRoots_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;
	std::unique_ptr<BlockHeader> pTipHeader = pServer->m_pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
	if (pTipHeader != nullptr)
	{
		Json::Value rootNode;
		rootNode["output_root_hash"] = HexUtil::ConvertToHex(pTipHeader->GetOutputRoot().GetData());
		rootNode["range_proof_root_hash"] = HexUtil::ConvertToHex(pTipHeader->GetRangeProofRoot().GetData());
		rootNode["kernel_root_hash"] = HexUtil::ConvertToHex(pTipHeader->GetKernelRoot().GetData());

		return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find tip.");
	}
}

int TxHashSetAPI::GetLastKernels_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	uint64_t numHashes = 10;

	const std::string queryString = RestUtil::GetQueryString(conn);
	if (!queryString.empty())
	{
		if (!StringUtil::StartsWith(queryString, "n="))
		{
			return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/lastkernels?n=###");
		}

		std::string numHashesStr = queryString.substr(2);
		numHashes = std::stoull(numHashesStr);
	}

	const ITxHashSet* pTxHashSet = pServer->m_pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet != nullptr)
	{
		Json::Value rootNode;

		std::vector<Hash> hashes = pTxHashSet->GetLastKernelHashes(numHashes);
		for (const Hash& hash : hashes)
		{
			Json::Value kernelNode;
			kernelNode["hash"] = HexUtil::ConvertToHex(hash.GetData());
			rootNode.append(kernelNode);
		}

		return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find TxHashSet.");
	}
}

int TxHashSetAPI::GetLastOutputs_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	uint64_t numHashes = 10;

	const std::string queryString = RestUtil::GetQueryString(conn);
	if (!queryString.empty())
	{
		if (!StringUtil::StartsWith(queryString, "n="))
		{
			return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/lastoutputs?n=###");
		}

		std::string numHashesStr = queryString.substr(2);
		numHashes = std::stoull(numHashesStr);
	}

	const ITxHashSet* pTxHashSet = pServer->m_pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet != nullptr)
	{
		Json::Value rootNode;

		std::vector<Hash> hashes = pTxHashSet->GetLastOutputHashes(numHashes);
		for (const Hash& hash : hashes)
		{
			Json::Value outputNode;
			outputNode["hash"] = HexUtil::ConvertToHex(hash.GetData());
			rootNode.append(outputNode);
		}

		return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find TxHashSet.");
	}
}

int TxHashSetAPI::GetLastRangeproofs_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	uint64_t numHashes = 10;

	const std::string queryString = RestUtil::GetQueryString(conn);
	if (!queryString.empty())
	{
		if (!StringUtil::StartsWith(queryString, "n="))
		{
			return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/lastrangeproofs?n=###");
		}

		std::string numHashesStr = queryString.substr(2);
		numHashes = std::stoull(numHashesStr);
	}

	const ITxHashSet* pTxHashSet = pServer->m_pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet != nullptr)
	{
		Json::Value rootNode;

		std::vector<Hash> hashes = pTxHashSet->GetLastRangeProofHashes(numHashes);
		for (const Hash& hash : hashes)
		{
			Json::Value rangeProofNode;
			rangeProofNode["hash"] = HexUtil::ConvertToHex(hash.GetData());
			rootNode.append(rangeProofNode);
		}

		return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find TxHashSet.");
	}
}

// get txhashset/outputs?start_index=1&max=100

/*
{
  "highest_index": 440211,
  "last_retrieved_index": 2647,
  "outputs": [
	{
	  "output_type": "Coinbase",
	  "commit": "08ec5b92bd00c558d29102761341006cac5c8183c4a2f31f3be6d59be9a4e576c5",
	  "spent": false,
	  "proof": "faba266f506814670b52a725e8df57225299e9681ae4129fd125492ca3e44c142175f60084f9dfcca7ee0364e1250c4170ba83e6e981d57178fb41ead6569814044540099ff328690bec5dcecc087adac1aab3f78ea009c6c68b75363acb39d836cad14a52ab0cea71045ae51ac4e7e54feeb052d08b1a0616c89e9c4768b575cdef8cd7505a3f239c7b2cece742a62b12ca0fb85a5887f4b75bf39b4d9ba40845411ba1c81435f2e378358aaf3097360af033a21f918021f75322f69dfdd415f4c1be46448670f9739498f0126f0f5875ed32258b62fc0d71e9d30a130877e33e0e928077f8d793092acd51edb82e95c851977dafaaff8c3676c5ebdc000fe9197db1ffad41f80eb1502b6679f409f671cd17096a9adf370cb571db3937b022ca8c9e70efb0773653d46a6bcb29f77b86f1f1422780e72a4a1d0c7d0f97b00617aa3f198432b06e39d55be1287639b3bf1e501962a87e9a4f1bd0ef3d55ff728119035933995eef4aadffc04936a935302cbe8cfc3e3757b45d3f6cc7a03985877e112c885d8d2299dc944a3ffe891cd021155b850f526a7b19671eebff8784200fbc86a3c8cd8663ee5eed74732af73654d1ff75edbff82a325c93ed983d00f69b4f48c57be7153cbf7898dd9b51056e1533c6a1fe251fe89864a55530b0ed0296e1ebbd50841701ece91425707c4a1f7a403e0f9f1e2a0acf4473e7680b34068e43b1182dae3d12b712352aa05d32f255365510a578c26a56829f8036e56623ff5a7f94f53e7999b81d7515c540a9952cd5e100fb35c84bbcdcac6a0fdbac34fa38a8dfede7b354242b7b43d343deaa604d28a62762794f4e86b30dcddc1bdc5d255770833f1980143d01c4b74a8894b0a597beb04dc3cb681b6d75d1ba2f55e820e0042013be5dbe5cb1955525af97422d5f29ed7612f3c3e37e2b0149d7139782",
	  "proof_hash": "2fe275068bbbf261c0191e111467af2bb0dad8bf3255d5418ee7df6152b5f73c",
	  "block_height": 503,
	  "merkle_proof": null,
	  "mmr_index": 999
	},
*/
int TxHashSetAPI::GetOutputs_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	uint64_t startIndex = 1;
	uint64_t max = 100;

	const std::string queryString = RestUtil::GetQueryString(conn);
	if (!queryString.empty())
	{
		std::vector<std::string> tokens = StringUtil::Split(queryString, "&");
		for (const std::string& token : tokens)
		{
			if (StringUtil::StartsWith(token, "start_index="))
			{
				std::vector<std::string> startIndexTokens = StringUtil::Split(token, "=");
				if (startIndexTokens.size() != 2)
				{
					return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/outputs?start_index=1&max=100");
				}

				startIndex = std::stoull(startIndexTokens[1]);
			}
			else if (StringUtil::StartsWith(token, "max="))
			{
				std::vector<std::string> maxTokens = StringUtil::Split(token, "=");
				if (maxTokens.size() != 2)
				{
					return RestUtil::BuildBadRequestResponse(conn, "Expected /v1/txhashset/outputs?start_index=1&max=100");
				}

				max = std::stoull(maxTokens[1]);
			}
		}
	}

	if (max > 1000)
	{
		max = 1000;
	}

	const ITxHashSet* pTxHashSet = pServer->m_pTxHashSetManager->GetTxHashSet();
	if (pTxHashSet != nullptr)
	{
		Json::Value rootNode;

		OutputRange range = pTxHashSet->GetOutputsByLeafIndex(startIndex, max);
		rootNode["highest_index"] = range.GetHighestIndex();
		rootNode["last_retrieved_index"] = range.GetLastRetrievedIndex();

		Json::Value outputsNode;
		for (const OutputDisplayInfo& info : range.GetOutputs())
		{
			Json::Value outputNode;
			const EOutputFeatures features = info.GetIdentifier().GetFeatures();
			outputNode["output_type"] = features == DEFAULT_OUTPUT ? "Transaction" : "Coinbase";
			outputNode["commit"] = info.GetIdentifier().GetCommitment().ToHex();
			outputNode["spent"] = info.IsSpent();
			outputNode["proof"] = HexUtil::ConvertToHex(info.GetRangeProof().GetProofBytes());

			Serializer proofSerializer;
			info.GetRangeProof().Serialize(proofSerializer);
			outputNode["proof_hash"] = HexUtil::ConvertToHex(Crypto::Blake2b(proofSerializer.GetBytes()).GetData());

			outputNode["block_height"] = info.GetLocation().GetBlockHeight();
			outputNode["merkle_proof"] = Json::nullValue;
			outputNode["mmr_index"] = info.GetLocation().GetMMRIndex() + 1;
			outputsNode.append(outputNode);
		}

		rootNode["outputs"] = outputsNode;

		return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(conn, "Failed to find TxHashSet.");
	}
}