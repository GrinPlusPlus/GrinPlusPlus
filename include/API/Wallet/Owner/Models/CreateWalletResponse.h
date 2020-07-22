#pragma once

#include <Net/Tor/TorAddress.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Traits/Jsonable.h>
#include <optional>

class CreateWalletResponse : public Traits::IJsonable
{
public:
    CreateWalletResponse(
        const SessionToken& token,
        const uint16_t listenerPort,
        const SecureString& walletSeed,
        const SlatepackAddress& address,
        const std::optional<TorAddress>& torAddressOpt
    ) : m_token(token),
        m_listenerPort(listenerPort),
        m_walletSeed(walletSeed),
        m_address(address),
        m_torAddress(torAddressOpt) { }
    virtual ~CreateWalletResponse() = default;

    const SessionToken& GetToken() const noexcept { return m_token; }
    uint16_t GetListenerPort() const noexcept { return m_listenerPort; }
    const SecureString& GetSeed() const noexcept { return m_walletSeed; }
    const SlatepackAddress& GetAddress() const noexcept { return m_address; }
    const std::optional<TorAddress>& GetTorAddress() const noexcept { return m_torAddress; }

    Json::Value ToJSON() const noexcept final
    {
        Json::Value result;
        result["session_token"] = m_token.ToBase64();
        result["wallet_seed"] = m_walletSeed.c_str();
        result["listener_port"] = m_listenerPort;
        result["slatepack_address"] = m_address.ToString();

        if (m_torAddress.has_value())
        {
            result["tor_address"] = m_torAddress.value().ToString();
        }

        return result;
    }

private:
    SessionToken m_token;
    uint16_t m_listenerPort;
    SecureString m_walletSeed;
    SlatepackAddress m_address;
    std::optional<TorAddress> m_torAddress;
};