#pragma once

#include <BlockChain/BlockChainServer.h>
#include <string>

// Forward Declarations
struct mg_connection;

class BlockAPI
{
public:
	static int GetBlock_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static std::unique_ptr<FullBlock> GetBlock(const std::string& requestedBlock, IBlockChainServerPtr pBlockChainServer);
};