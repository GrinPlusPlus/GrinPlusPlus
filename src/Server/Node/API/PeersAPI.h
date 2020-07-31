#pragma once

#include <string>

// Forward Declarations
struct mg_connection;

class PeersAPI
{
public:
	static int GetAllPeers_Handler(struct mg_connection* conn, void* pNodeContext);
	static int GetConnectedPeers_Handler(struct mg_connection* conn, void* pNodeContext);
	static int Peer_Handler(struct mg_connection* conn, void* pNodeContext);

private:
	static std::string ParseIPAddress(const std::string& request);
	static std::string ParseCommand(const std::string& request);
};