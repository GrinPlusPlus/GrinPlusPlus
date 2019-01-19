#pragma once

#include "../civetweb/include/civetweb.h"

class ServerAPI
{
public:
	static int GetServer_Handler(struct mg_connection* conn, void* pServerContainer);
};