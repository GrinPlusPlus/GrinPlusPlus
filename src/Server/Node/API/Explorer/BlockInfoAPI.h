#pragma once

#include "../../civetweb/include/civetweb.h"

// Forward Declarations
struct NodeContext;

class BlockInfoAPI
{
public:
	static int GetBlockInfo_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static int GetLatestBlockInfo(struct mg_connection* conn, NodeContext& server);
};