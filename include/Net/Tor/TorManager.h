#pragma once

#include <Config/TorConfig.h>
#include <Crypto/SecretKey.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorConnection.h>

// Forward Declarations
class TorControl;

class TorManager
{
  public:
    static TorManager &GetInstance(const TorConfig &config);

    std::unique_ptr<TorAddress> AddListener(const SecretKey &privateKey, const int portNumber);
    bool RemoveListener(const TorAddress &torAddress);

    std::unique_ptr<TorConnection> Connect(const TorAddress &address);

  private:
    TorManager(const TorConfig &config);
    ~TorManager();

    const TorConfig &m_torConfig;
    TorControl *m_pControl;
    asio::io_context m_context;
};