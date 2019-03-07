#pragma once

#include "../../civetweb/include/civetweb.h"

#include <Wallet/SessionToken.h>
#include <json/json.h>

// Forward Declarations
class IWalletManager;
class INodeClient;
class SessionToken;
struct WalletContext;

class OwnerAPI
{
public:
	static int Handler(mg_connection* pConnection, void* pWalletContext);

private:
	static int HandleGET(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient);
	static int HandlePOST(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient);

	static SessionToken GetSessionToken(mg_connection* pConnection);

	static int CreateWallet(mg_connection* pConnection, IWalletManager& walletManager);
	static int Login(mg_connection* pConnection, IWalletManager& walletManager);
	static int Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);

	static int RetrieveSummaryInfo(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);

	static int Send(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
	static int Receive(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
	static int Finalize(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
};