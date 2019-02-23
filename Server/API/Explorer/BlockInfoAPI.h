#pragma once

#include "../../civetweb/include/civetweb.h"

// Forward Declarations
struct ServerContainer;

class BlockInfoAPI
{
public:
	static int GetBlockInfo_Handler(struct mg_connection* conn, void* pServerContainer);

private:
	static int GetLatestBlockInfo(struct mg_connection* conn, ServerContainer& server);
};