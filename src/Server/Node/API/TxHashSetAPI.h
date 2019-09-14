#pragma once

#include "../../civetweb/include/civetweb.h"

class TxHashSetAPI
{
public:
	static int GetRoots_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetLastKernels_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetLastOutputs_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetLastRangeproofs_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetOutputs_Handler(struct mg_connection* conn, void* pNodeContext);
};