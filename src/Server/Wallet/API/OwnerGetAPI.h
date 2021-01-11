#pragma once

#include <string>

// Forward Declarations
struct mg_connection;
class IWalletManager;

class OwnerGetAPI
{
public:
	static int HandleGET(
		mg_connection* pConnection,
		const std::string& action,
		IWalletManager& walletManager
	);
};