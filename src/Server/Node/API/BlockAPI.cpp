#include "BlockAPI.h"
#include "../../JSONFactory.h"
#include "../NodeContext.h"

#include <Net/Util/HTTPUtil.h>
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
	const std::string requestedBlock = HTTPUtil::GetURIParam(conn, "/v1/blocks/");
	const std::string queryString = HTTPUtil::GetQueryString(conn);

	IBlockChainServerPtr pBlockChainServer = ((NodeContext*)pNodeContext)->m_pBlockChainServer;
	if (queryString == "compact")
	{
		std::unique_ptr<FullBlock> pBlock = GetBlock(requestedBlock, pBlockChainServer);

		if (nullptr != pBlock)
		{
			std::unique_ptr<CompactBlock> pCompactBlock = pBlockChainServer->GetCompactBlockByHash(pBlock->GetHash());
			if (pCompactBlock != nullptr)
			{
				const Json::Value compactBlockJSON = JSONFactory::BuildCompactBlockJSON(*pCompactBlock);
				return HTTPUtil::BuildSuccessResponse(conn, compactBlockJSON.toStyledString());
			}
		}
	}
	else
	{
		std::unique_ptr<FullBlock> pFullBlock = GetBlock(requestedBlock, pBlockChainServer);

		if (nullptr != pFullBlock)
		{
			const Json::Value blockJSON = JSONFactory::BuildBlockJSON(*pFullBlock);
			return HTTPUtil::BuildSuccessResponse(conn, blockJSON.toStyledString());
		}
	}

	const std::string response = "BLOCK NOT FOUND";
	return HTTPUtil::BuildBadRequestResponse(conn, response);
}

std::unique_ptr<FullBlock> BlockAPI::GetBlock(const std::string& requestedBlock, IBlockChainServerPtr pBlockChainServer)
{
	if (requestedBlock.length() == 64 && HexUtil::IsValidHex(requestedBlock))
	{
		try
		{
			const Hash hash = Hash::FromHex(requestedBlock);
			std::unique_ptr<FullBlock> pBlock = pBlockChainServer->GetBlockByHash(hash);
			if (pBlock != nullptr)
			{
				LOG_DEBUG_F("Found block with hash {}.", requestedBlock);
				return pBlock;
			}
			else
			{
				LOG_INFO_F("No block found with hash {}.", requestedBlock);
			}
		}
		catch (const std::exception&)
		{
			LOG_ERROR_F("Failed converting {} to a Hash.", requestedBlock);
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
				LOG_DEBUG_F("Found block with output commitment {}.", requestedBlock);
				return pBlock;
			}
			else
			{
				LOG_INFO_F("No block found with commitment {}.", requestedBlock);
			}
		}
		catch (const std::exception&)
		{
			LOG_ERROR_F("Failed converting {} to a Commitment.", requestedBlock);
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
				LOG_INFO_F("Found block at height {}.", requestedBlock);
				return pBlock;
			}
			else
			{
				LOG_INFO_F("No block found at height {}.", requestedBlock);
			}
		}
		catch (const std::invalid_argument&)
		{
			LOG_ERROR_F("Failed converting {} to height.", requestedBlock);
		}
	}

	return std::unique_ptr<FullBlock>(nullptr);
}