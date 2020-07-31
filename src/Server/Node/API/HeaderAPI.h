#pragma once

#include <BlockChain/BlockChainServer.h>
#include <string>

// Forward Declarations
struct mg_connection;

class HeaderAPI
{
public:
	static int GetHeader_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static BlockHeaderPtr GetHeader(const std::string& requestedHeader, IBlockChainServerPtr pBlockChainServer);
};