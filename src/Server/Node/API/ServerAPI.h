#pragma once

#include "../../civetweb/include/civetweb.h"
#include <string>

// Forward Declarations
class SyncStatus;

class ServerAPI
{
public:
	static int V1_Handler(struct mg_connection* conn, void* pVoid);
	static int GetStatus_Handler(struct mg_connection* conn, void* pNodeContext);
	static int ResyncChain_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static std::string GetStatusString(const SyncStatus& syncStatus);
};