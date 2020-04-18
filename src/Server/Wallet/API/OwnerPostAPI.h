#pragma once

#include <civetweb.h>
#include <Config/Config.h>
#include <Net/Tor/TorProcess.h>
#include <Wallet/SessionToken.h>
#include <json/json.h>

// Forward Declarations
class IWalletManager;
class SessionToken;

class OwnerPostAPI
{
public:
	OwnerPostAPI(const Config& config, const TorProcess::Ptr& pTorProcess);

	int HandlePOST(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager);

private:
	int CreateWallet(mg_connection* pConnection, IWalletManager& walletManager);
	int Login(mg_connection* pConnection, IWalletManager& walletManager);
	int Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	int RestoreWallet(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json);
	int UpdateWallet(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);

	int Send(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json);
	int Receive(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json);
	int Finalize(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json);
	int Repost(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	int Cancel(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);

	int EstimateFee(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json);

	const Config& m_config;
	TorProcess::Ptr m_pTorProcess;
};