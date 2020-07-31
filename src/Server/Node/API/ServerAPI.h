#pragma once

#include <string>

// Forward Declarations
struct mg_connection;
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