#pragma once

#include "../civetweb/include/civetweb.h"

class TxHashSetAPI
{
public:
	static int GetRoots_Handler(struct mg_connection* conn, void* pServerContainer);
	static int GetLastKernels_Handler(struct mg_connection* conn, void* pServerContainer);
	static int GetLastOutputs_Handler(struct mg_connection* conn, void* pServerContainer);
	static int GetLastRangeproofs_Handler(struct mg_connection* conn, void* pServerContainer);
	static int GetOutputs_Handler(struct mg_connection* conn, void* pServerContainer);
};