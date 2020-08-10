#include "HeaderAPI.h"
#include "../NodeContext.h"

#include <Net/Util/HTTPUtil.h>
#include <BlockChain/BlockChain.h>
#include <Common/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/HexUtil.h>
#include <string>

//
// Handles requests to retrieve a single header by hash, height, or output commitment.
//
// APIs:
// GET /v1/headers/<hash>
// GET /v1/headers/<height>
// GET /v1/headers/<output commit>
//
int HeaderAPI::GetHeader_Handler(struct mg_connection* conn, void* pNodeContext)
{
	try
	{
		const std::string requestedHeader = HTTPUtil::GetURIParam(conn, "/v1/headers/");
		auto pBlockHeader = GetHeader(requestedHeader, ((NodeContext*)pNodeContext)->m_pBlockChain);

		if (nullptr != pBlockHeader)
		{
			return HTTPUtil::BuildSuccessResponse(conn, pBlockHeader->ToJSON().toStyledString());
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	const std::string response = "HEADER NOT FOUND";
	return HTTPUtil::BuildBadRequestResponse(conn, response);
}

BlockHeaderPtr HeaderAPI::GetHeader(const std::string& requestedHeader, const IBlockChain::Ptr& pBlockChain)
{
	if (requestedHeader.length() == 64 && HexUtil::IsValidHex(requestedHeader))
	{
		try
		{
			const Hash hash = Hash::FromHex(requestedHeader);
			auto pHeader = pBlockChain->GetBlockHeaderByHash(hash);
			if (pHeader != nullptr)
			{
				LOG_INFO_F("Found header with hash {}.", requestedHeader);
				return pHeader;
			}
			else
			{
				LOG_INFO_F("No header found with hash {}.", requestedHeader);
			}
		}
		catch (const std::exception&)
		{
			LOG_ERROR_F("Failed converting {} to a Hash.", requestedHeader);
		}
	}
	else if (requestedHeader.length() == 66 && HexUtil::IsValidHex(requestedHeader))
	{
		try
		{
			const Commitment outputCommitment(CBigInteger<33>::FromHex(requestedHeader));
			auto pHeader = pBlockChain->GetBlockHeaderByCommitment(outputCommitment);
			if (pHeader != nullptr)
			{
				LOG_INFO_F("Found header with output commitment {}.", requestedHeader);
				return pHeader;
			}
			else
			{
				LOG_INFO_F("No header found with commitment {}.", requestedHeader);
			}
		}
		catch (const std::exception&)
		{
			LOG_ERROR_F("Failed converting {} to a Commitment.", requestedHeader);
		}
	}
	else
	{
		try
		{
			std::string::size_type sz = 0;
			const uint64_t height = std::stoull(requestedHeader, &sz, 0);

			auto pHeader = pBlockChain->GetBlockHeaderByHeight(height, EChainType::CANDIDATE);
			if (pHeader != nullptr)
			{
				LOG_INFO_F("Found header at height {}.", requestedHeader);
				return pHeader;
			}
			else
			{
				LOG_INFO_F("No header found at height {}.", requestedHeader);
			}
		}
		catch (const std::invalid_argument&)
		{
			LOG_ERROR_F("Failed converting {} to height.", requestedHeader);
		}
	}

	return nullptr;
}