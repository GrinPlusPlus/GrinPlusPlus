#pragma once

#include <BlockChain/BlockChain.h>
#include <string>

// Forward Declarations
struct mg_connection;

class HeaderAPI
{
public:
	static int GetHeader_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static BlockHeaderPtr GetHeader(const std::string& requestedHeader, const IBlockChain::Ptr& pBlockChain);
};