#pragma once

#include "../../civetweb/include/civetweb.h"

#include <json/json.h>

// Forward Declarations
class IWalletManager;
class SessionToken;

class OwnerAPI
{
public:
	static int Handler(mg_connection* pConnection, void* pWalletContext);

private:
	static int CreateWallet(mg_connection* pConnection, IWalletManager& walletManager);
	static int Login(mg_connection* pConnection, IWalletManager& walletManager);
	static int Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);

	static int Send(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
};