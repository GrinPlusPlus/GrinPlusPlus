#pragma once

#include "../civetweb/include/civetweb.h"

class TxHashSetAPI
{
public:
	static int GetTxHashSet_Handler(struct mg_connection* conn, void* pServerContainer);
};