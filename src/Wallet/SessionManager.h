#pragma once

#include "ForeignController.h"
#include "LockedWallet.h"
#include "LoggedInSession.h"
#include "Wallet.h"

#include <Common/Secure.h>
#include <Config/Config.h>
#include <Crypto/SecretKey.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SessionToken.h>
#include <memory>
#include <unordered_map>

// Forward Declarations
class IWalletManager;

class SessionManager
{
  public:
    SessionManager(const Config &config, const INodeClient &nodeClient, IWalletDB &walletDB,
                   IWalletManager &walletManager);
    ~SessionManager();

    std::unique_ptr<SessionToken> Login(const std::string &username, const SecureString &password);
    SessionToken Login(const std::string &username, const SecureVector &seed);
    void Logout(const SessionToken &token);

    SecureVector GetSeed(const SessionToken &token) const;
    SecretKey GetGrinboxAddress(const SessionToken &token) const;
    LockedWallet GetWallet(const SessionToken &token);

  private:
    std::unordered_map<uint64_t, LoggedInSession *> m_sessionsById;
    // TODO: Keep multimap of sessions per username

    uint64_t m_nextSessionId;

    const Config &m_config;
    const INodeClient &m_nodeClient;
    IWalletDB &m_walletDB;
    ForeignController m_foreignController;
};