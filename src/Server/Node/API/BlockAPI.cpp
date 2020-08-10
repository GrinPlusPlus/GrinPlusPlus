#include "BlockAPI.h"
#include "../NodeContext.h"

#include <Net/Util/HTTPUtil.h>
#include <Core/Models/CompactBlock.h>
#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>

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
	try
	{
		const std::string requestedBlock = HTTPUtil::GetURIParam(conn, "/v1/blocks/");
		const std::string queryString = HTTPUtil::GetQueryString(conn);

		IBlockChain::Ptr pBlockChain = ((NodeContext*)pNodeContext)->m_pBlockChain;
		if (queryString == "compact")
		{
			std::unique_ptr<FullBlock> pBlock = GetBlock(requestedBlock, pBlockChain);
			if (pBlock != nullptr)
			{
				std::unique_ptr<CompactBlock> pCompactBlock =
					pBlockChain->GetCompactBlockByHash(pBlock->GetHash());
				if (pCompactBlock != nullptr)
				{
					return HTTPUtil::BuildSuccessResponse(conn, pCompactBlock->ToJSON().toStyledString());
				}
			}
		}
		else
		{
			std::unique_ptr<FullBlock> pFullBlock = GetBlock(requestedBlock, pBlockChain);
			if (pFullBlock != nullptr)
			{
				return HTTPUtil::BuildSuccessResponse(conn, pFullBlock->ToJSON().toStyledString());
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	return HTTPUtil::BuildBadRequestResponse(conn, "BLOCK NOT FOUND");
}

std::unique_ptr<FullBlock> BlockAPI::GetBlock(
	const std::string& requestedBlock,
	const IBlockChain::Ptr& pBlockChain)
{
	if (requestedBlock.length() == 64 && HexUtil::IsValidHex(requestedBlock))
	{
		try
		{
			std::unique_ptr<FullBlock> pBlock =
				pBlockChain->GetBlockByHash(Hash::FromHex(requestedBlock));
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
			std::unique_ptr<FullBlock> pBlock =
				pBlockChain->GetBlockByCommitment(Commitment::FromHex(requestedBlock));
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

			std::unique_ptr<FullBlock> pBlock = pBlockChain->GetBlockByHeight(height);
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