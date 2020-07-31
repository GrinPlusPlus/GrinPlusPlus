#pragma once

// Forward Declarations
struct mg_connection;

class ChainAPI
{
public:
	static int GetChain_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetChainOutputsByHeight_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetChainOutputsByIds_Handler(struct mg_connection* conn, void* pNodeContext);
};