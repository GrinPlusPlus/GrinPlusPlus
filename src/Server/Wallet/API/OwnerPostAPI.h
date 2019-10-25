#pragma once

#include <Config/Config.h>
#include <Wallet/SessionToken.h>
#include <civetweb.h>
#include <json/json.h>

// Forward Declarations
class IWalletManager;
class INodeClient;
class SessionToken;

class OwnerPostAPI
{
  public:
    OwnerPostAPI(const Config &config);

    int HandlePOST(mg_connection *pConnection, const std::string &action, IWalletManager &walletManager,
                   INodeClient &nodeClient);

  private:
    int CreateWallet(mg_connection *pConnection, IWalletManager &walletManager);
    int Login(mg_connection *pConnection, IWalletManager &walletManager);
    int Logout(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token);
    int RestoreWallet(mg_connection *pConnection, IWalletManager &walletManager, const Json::Value &json);
    int UpdateWallet(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token);

    int Send(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token,
             const Json::Value &json);
    int Receive(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token,
                const Json::Value &json);
    int Finalize(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token,
                 const Json::Value &json);
    int PostTx(mg_connection *pConnection, INodeClient &nodeClient, const SessionToken &token, const Json::Value &json);
    int Repost(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token);
    int Cancel(mg_connection *pConnection, IWalletManager &walletManager, const SessionToken &token);

    const Config &m_config;
};