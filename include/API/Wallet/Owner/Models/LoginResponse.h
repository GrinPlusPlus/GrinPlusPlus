#pragma once

#include <Net/Tor/TorAddress.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Traits/Jsonable.h>
#include <optional>

class LoginResponse : public Traits::IJsonable
{
public:
    LoginResponse(
        const SessionToken& token,
        const uint16_t listenerPort,
        const SlatepackAddress& address,
        const std::optional<TorAddress>& torAddressOpt
    ) : m_token(token), m_listenerPort(listenerPort), m_address(address), m_torAddress(torAddressOpt) { }
    virtual ~LoginResponse() = default;

    const SessionToken& GetToken() const noexcept { return m_token; }
    uint16_t GetPort() const noexcept { return m_listenerPort; }
    SlatepackAddress GetAddress() const noexcept { return m_address; }
    const std::optional<TorAddress>& GetTorAddress() const noexcept { return m_torAddress; }

    Json::Value ToJSON() const noexcept final
    {
        Json::Value result;
        result["session_token"] = m_token.ToBase64();
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
    SlatepackAddress m_address;
    std::optional<TorAddress> m_torAddress;
};