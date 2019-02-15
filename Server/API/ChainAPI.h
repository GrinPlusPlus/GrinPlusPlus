#pragma once

#include "../civetweb/include/civetweb.h"

class ChainAPI
{
public:
	static int GetChain_Handler(struct mg_connection* conn, void* pBlockChainServer);
	static int GetChainOutputsByHeight_Handler(struct mg_connection* conn, void* pServerContainer);
	static int GetChainOutputsByIds_Handler(struct mg_connection* conn, void* pServerContainer);
};