#include "BlockAPI.h"
#include "../../RestUtil.h"
#include "../../JSONFactory.h"
#include "../NodeContext.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

//
// Handles requests to retrieve a single block by hash, height, or output commitment.
//
// APIs:
// GET /v1/blocks/<hash>
// GET /v1/blocks/<height>
// GET /v1/blocks/<output commit>
//
// Return results as "compact blocks" by passing "?compact" query
// GET /v1/blocks/<hash>?compact
int BlockAPI::GetBlock_Handler(struct mg_connection* conn, void* pNodeContext)
{
	const std::string requestedBlock = RestUtil::GetURIParam(conn, "/v1/blocks/");
	const std::string queryString = RestUtil::GetQueryString(conn);

	IBlockChainServer* pBlockChainServer = ((NodeContext*)pNodeContext)->m_pBlockChainServer;
	if (queryString == "compact")
	{
		std::unique_ptr<FullBlock> pBlock = GetBlock(requestedBlock, pBlockChainServer);

		if (nullptr != pBlock)
		{
			std::unique_ptr<CompactBlock> pCompactBlock = pBlockChainServer->GetCompactBlockByHash(pBlock->GetHash());
			if (pCompactBlock != nullptr)
			{
				const Json::Value compactBlockJSON = JSONFactory::BuildCompactBlockJSON(*pCompactBlock);
				return RestUtil::BuildSuccessResponse(conn, compactBlockJSON.toStyledString());
			}
		}
	}
	else
	{
		std::unique_ptr<FullBlock> pFullBlock = GetBlock(requestedBlock, pBlockChainServer);

		if (nullptr != pFullBlock)
		{
			const Json::Value blockJSON = JSONFactory::BuildBlockJSON(*pFullBlock);
			return RestUtil::BuildSuccessResponse(conn, blockJSON.toStyledString());
		}
	}

	const std::string response = "BLOCK NOT FOUND";
	return RestUtil::BuildBadRequestResponse(conn, response);
}

std::unique_ptr<FullBlock> BlockAPI::GetBlock(const std::string& requestedBlock, IBlockChainServer* pBlockChainServer)
{
	if (requestedBlock.length() == 64 && HexUtil::IsValidHex(requestedBlock))
	{
		try
		{
			const Hash hash = Hash::FromHex(requestedBlock);
			std::unique_ptr<FullBlock> pBlock = pBlockChainServer->GetBlockByHash(hash);
			if (pBlock != nullptr)
			{
				LoggerAPI::LogDebug(StringUtil::Format("BlockAPI::GetBlock - Found block with hash %s.", requestedBlock.c_str()));
				return pBlock;
			}
			else
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - No block found with hash %s.", requestedBlock.c_str()));
			}
		}
		catch (const std::exception&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetBlock - Failed converting %s to a Hash.", requestedBlock.c_str()));
		}
	}
	else if (requestedBlock.length() == 66 && HexUtil::IsValidHex(requestedBlock))
	{
		try
		{
			const Commitment outputCommitment = Commitment(CBigInteger<33>::FromHex(requestedBlock));
			std::unique_ptr<FullBlock> pBlock = pBlockChainServer->GetBlockByCommitment(outputCommitment);
			if (pBlock != nullptr)
			{
				LoggerAPI::LogDebug(StringUtil::Format("BlockAPI::GetBlock - Found block with output commitment %s.", requestedBlock.c_str()));
				return pBlock;
			}
			else
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - No block found with commitment %s.", requestedBlock.c_str()));
			}
		}
		catch (const std::exception&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetBlock - Failed converting %s to a Commitment.", requestedBlock.c_str()));
		}
	}
	else
	{
		try
		{
			std::string::size_type sz = 0;
			const uint64_t height = std::stoull(requestedBlock, &sz, 0);

			std::unique_ptr<FullBlock> pBlock = pBlockChainServer->GetBlockByHeight(height);
			if (pBlock != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - Found block at height %s.", requestedBlock.c_str()));
				return pBlock;
			}
			else
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - No block found at height %s.", requestedBlock.c_str()));
			}
		}
		catch (const std::invalid_argument&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetBlock - Failed converting %s to height.", requestedBlock.c_str()));
		}
	}

	return std::unique_ptr<FullBlock>(nullptr);
}