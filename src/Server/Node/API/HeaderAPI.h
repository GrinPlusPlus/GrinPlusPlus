#pragma once

#include "../../civetweb/include/civetweb.h"

#include <BlockChain/BlockChainServer.h>
#include <string>

class HeaderAPI
{
public:
	static int GetHeader_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static BlockHeaderPtr GetHeader(const std::string& requestedHeader, IBlockChainServerPtr pBlockChainServer);
};