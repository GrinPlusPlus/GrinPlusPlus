#pragma once

#include "../../civetweb/include/civetweb.h"

#include <Wallet/SessionToken.h>
#include <json/json.h>

// Forward Declarations
class IWalletManager;
class INodeClient;
class SessionToken;

class OwnerPostAPI
{
public:
	static int HandlePOST(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient);

private:
	static int CreateWallet(mg_connection* pConnection, IWalletManager& walletManager);
	static int Login(mg_connection* pConnection, IWalletManager& walletManager);
	static int Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	static int RestoreWallet(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json);
	static int UpdateWallet(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);

	static int Send(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
	static int Receive(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
	static int Finalize(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);
	static int PostTx(mg_connection* pConnection, INodeClient& nodeClient, const SessionToken& token, const Json::Value& json);
	static int Repost(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	static int Cancel(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
};