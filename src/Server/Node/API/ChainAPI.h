#pragma once

#include "../../civetweb/include/civetweb.h"

class ChainAPI
{
public:
	static int GetChain_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetChainOutputsByHeight_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetChainOutputsByIds_Handler(struct mg_connection* conn, void* pNodeContext);
};