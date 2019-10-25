#pragma once

#include "TorControlClient.h"

#include <Config/TorConfig.h>
#include <Crypto/SecretKey.h>
#include <Net/Tor/TorAddress.h>

class TorControl
{
  public:
    TorControl(const TorConfig &torConfig);

    bool Initialize();

    void Shutdown();

    std::string AddOnion(const SecretKey &privateKey, const uint16_t externalPort, const uint16_t internalPort);
    bool DelOnion(const TorAddress &torAddress);

  private:
    bool Authenticate(const std::string &password);

    std::string FormatKey(const SecretKey &privateKey) const;

    const TorConfig &m_torConfig;
    std::string m_password;
    long m_processId;

    TorControlClient m_client;
};