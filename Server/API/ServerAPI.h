#pragma once

#include "../civetweb/include/civetweb.h"

class ServerAPI
{
public:
	static int V1_Handler(struct mg_connection* conn, void* pVoid);
	static int GetStatus_Handler(struct mg_connection* conn, void* pServerContainer);
};