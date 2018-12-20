#pragma once

#include "civetweb/include/civetweb.h"

#include <BlockChainServer.h>
#include <string>

class BlockAPI
{
public:
	static int GetHeader_Handler(struct mg_connection* conn, void* pBlockChainServer);

private:
	static std::unique_ptr<BlockHeader> GetHeader(const std::string& requestedHeader, IBlockChainServer* pBlockChainServer);
	static std::string BuildHeaderJSON(const BlockHeader& header);
};