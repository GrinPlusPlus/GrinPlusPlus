#pragma once

#include <Config/Config.h>
#include <Net/Tor/TorAddress.h>
#include <Wallet/SessionToken.h>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

// Forward Declarations
class IWalletManager;
struct mg_context;
struct mg_connection;

class ForeignController
{
  public:
    ForeignController(const Config &config, IWalletManager &walletManager);

    std::optional<TorAddress> StartListener(const std::string &username, const SessionToken &token,
                                            const SecureVector &seed);
    bool StopListener(const std::string &username);

  private:
    struct Context
    {
        Context(mg_context *pCivetContext, IWalletManager &walletManager, int portNumber, const SessionToken &token)
            : m_numReferences(1), m_walletManager(walletManager), m_pCivetContext(pCivetContext),
              m_portNumber(portNumber), m_token(token)
        {
        }

        int m_numReferences;
        IWalletManager &m_walletManager;
        mg_context *m_pCivetContext;
        SessionToken m_token;
        int m_portNumber;
        std::optional<TorAddress> m_torAddress;
    };

    static int ForeignAPIHandler(mg_connection *pConnection, void *pCbContext);

    const Config &m_config;
    IWalletManager &m_walletManager;

    mutable std::mutex m_contextsMutex;
    std::unordered_map<std::string, Context *> m_contextsByUsername;
};