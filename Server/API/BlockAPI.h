#pragma once

#include "../civetweb/include/civetweb.h"

#include <BlockChainServer.h>
#include <string>

class BlockAPI
{
public:
	static int GetBlock_Handler(struct mg_connection* conn, void* pBlockChainServer);

private:
	static std::unique_ptr<FullBlock> GetBlock(const std::string& requestedBlock, IBlockChainServer* pBlockChainServer);
	static std::string BuildBlockJSON(const FullBlock& block);

	static std::unique_ptr<CompactBlock> GetCompactBlock(const std::string& requestedBlock, IBlockChainServer* pBlockChainServer);
	static std::string BuildCompactBlockJSON(const CompactBlock& block);
};