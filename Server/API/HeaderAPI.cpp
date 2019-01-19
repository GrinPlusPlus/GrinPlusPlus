#include "HeaderAPI.h"
#include "../RestUtil.h"
#include "../JSONFactory.h"

#include <BlockChainServer.h>
#include <Infrastructure/Logger.h>
#include <StringUtil.h>
#include <HexUtil.h>
#include <string>

//
// Handles requests to retrieve a single header by hash, height, or output commitment.
//
// APIs:
// GET /v1/headers/<hash>
// GET /v1/headers/<height>
// GET /v1/headers/<output commit>
//
int HeaderAPI::GetHeader_Handler(struct mg_connection* conn, void* pBlockChainServer)
{
	const std::string requestedHeader = RestUtil::GetURIParam(conn, "/v1/headers/");
	std::unique_ptr<BlockHeader> pBlockHeader = GetHeader(requestedHeader, (IBlockChainServer*)pBlockChainServer);

	if (nullptr != pBlockHeader)
	{
		const Json::Value headerNode = JSONFactory::BuildHeaderJSON(*pBlockHeader);
		return RestUtil::BuildSuccessResponse(conn, headerNode.toStyledString());
	}
	else
	{
		const std::string response = "HEADER NOT FOUND";
		return RestUtil::BuildBadRequestResponse(conn, response);
	}
}

std::unique_ptr<BlockHeader> HeaderAPI::GetHeader(const std::string& requestedHeader, IBlockChainServer* pBlockChainServer)
{
	if (requestedHeader.length() == 64 && HexUtil::IsValidHex(requestedHeader))
	{
		try
		{
			const Hash hash = Hash::FromHex(requestedHeader);
			std::unique_ptr<BlockHeader> pHeader = pBlockChainServer->GetBlockHeaderByHash(hash);
			if (pHeader != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetHeader - Found header with hash %s.", requestedHeader.c_str()));
				return pHeader;
			}

			pHeader = pBlockChainServer->GetBlockHeaderByCommitment(hash);
			if (pHeader != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetHeader - Found header with output commitment %s.", requestedHeader.c_str()));
				return pHeader;
			}

			LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetHeader - No header found with hash or commitment %s.", requestedHeader.c_str()));
		}
		catch (const std::exception&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetHeader - Failed converting %s to a Hash.", requestedHeader.c_str()));
		}
	}
	else
	{
		try
		{
			std::string::size_type sz = 0;
			const uint64_t height = std::stoull(requestedHeader, &sz, 0);

			std::unique_ptr<BlockHeader> pHeader = pBlockChainServer->GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
			if (pHeader != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetHeader - Found header at height %s.", requestedHeader.c_str()));
				return pHeader;
			}
			else
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetHeader - No header found at height %s.", requestedHeader.c_str()));
			}
		}
		catch (const std::invalid_argument&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetHeader - Failed converting %s to height.", requestedHeader.c_str()));
		}
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}