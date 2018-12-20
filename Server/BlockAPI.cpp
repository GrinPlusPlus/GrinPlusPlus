#include "BlockAPI.h"

#include <json/json.h>
#include <BlockChainServer.h>
#include <Infrastructure/Logger.h>
#include <StringUtil.h>
#include <HexUtil.h>
#include <TimeUtil.h>
#include <string>

//
// Handles requests to retrieve a single header by hash, height, or output commitment.
//
// APIs:
// GET /v1/headers/<hash>
// GET /v1/headers/<height>
// GET /v1/headers/<output commit>
//
int BlockAPI::GetHeader_Handler(struct mg_connection* conn, void* pBlockChainServer)
{
	const struct mg_request_info* req_info = mg_get_request_info(conn);
	
	const std::string requestURI(req_info->request_uri);
	const std::string requestedHeader = requestURI.substr(12, requestURI.size() - 12);
	std::unique_ptr<BlockHeader> pBlockHeader = GetHeader(requestedHeader, (IBlockChainServer*)pBlockChainServer);

	if (nullptr != pBlockHeader)
	{
		const std::string msg = BuildHeaderJSON(*pBlockHeader);
		unsigned long len = (unsigned long)msg.size();

		mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: application/json\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, msg.c_str(), len);

		return 200;
	}
	else
	{
		const std::string msg = "HEADER NOT FOUND";
		unsigned long len = (unsigned long)msg.size();

		mg_printf(conn,
			"HTTP/1.1 400 Bad Request\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, msg.c_str(), len);

		return 400;
	}
}

std::unique_ptr<BlockHeader> BlockAPI::GetHeader(const std::string& requestedHeader, IBlockChainServer* pBlockChainServer)
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

std::string BlockAPI::BuildHeaderJSON(const BlockHeader& header)
{
	Json::Value rootNode;
	rootNode["Height"] = header.GetHeight();
	rootNode["Hash"] = HexUtil::ConvertToHex(header.GetHash().GetData(), false, false);
	rootNode["Version"] = header.GetVersion();

	rootNode["TimestampRaw"] = header.GetTimestamp();
	rootNode["TimestampLocal"] = TimeUtil::FormatLocal(header.GetTimestamp());
	rootNode["TimestampUTC"] = TimeUtil::FormatUTC(header.GetTimestamp());

	rootNode["PreviousHash"] = HexUtil::ConvertToHex(header.GetPreviousBlockHash().GetData(), false, false);
	rootNode["PreviousMMRRoot"] = HexUtil::ConvertToHex(header.GetPreviousRoot().GetData(), false, false);

	rootNode["KernelMMRRoot"] = HexUtil::ConvertToHex(header.GetKernelRoot().GetData(), false, false);
	rootNode["KernelMMRSize"] = header.GetKernelMMRSize();
	rootNode["TotalKernelOffset"] = HexUtil::ConvertToHex(header.GetTotalKernelOffset().GetBlindingFactorBytes().GetData(), false, false);

	rootNode["OutputMMRRoot"] = HexUtil::ConvertToHex(header.GetOutputRoot().GetData(), false, false);
	rootNode["OutputMMRSIze"] = header.GetOutputMMRSize();
	rootNode["RangeProofMMRRoot"] = HexUtil::ConvertToHex(header.GetRangeProofRoot().GetData(), false, false);

	// Proof Of Work
	const ProofOfWork& proofOfWork = header.GetProofOfWork();
	rootNode["ScalingDifficulty"] = proofOfWork.GetScalingDifficulty();
	rootNode["TotalDifficulty"] = proofOfWork.GetTotalDifficulty();
	rootNode["Nonce"] = proofOfWork.GetNonce();

	return rootNode.toStyledString();
}