#pragma once

#include <Common/Logger.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <Net/Tor/TorProcess.h>
#include <optional>

// Forward Declarations
class KeyChain;
class IWalletManager;
class SessionToken;

class ForeignServer
{
public:
    using UPtr = std::unique_ptr<ForeignServer>;

    ForeignServer(
        const std::shared_ptr<ITorProcess>& pTorProcess,
        const RPCServerPtr& pRPCServer,
        const std::optional<TorAddress>& torAddressOpt)
        : m_pTorProcess(pTorProcess),
        m_pRPCServer(pRPCServer),
        m_torAddress(torAddressOpt)
    {
        LOG_INFO("Starting foreign server");
    }

    ~ForeignServer()
    {
        LOG_INFO("Shutting down foreign server");
        if (m_torAddress.has_value())
        {
            m_pTorProcess->RemoveListener(m_torAddress.value());
        }
    }

    static ForeignServer::UPtr Create(
        const KeyChain& keyChain,
        const std::shared_ptr<ITorProcess>& pTorProcess,
        IWalletManager& walletManager,
        const SessionToken& token,
        const int currentAddressIndex
    );

    uint16_t GetPortNumber() const noexcept { return m_pRPCServer->GetPortNumber(); }
    const std::optional<TorAddress>& GetTorAddress() const noexcept { return m_torAddress; }

private:
    static std::optional<TorAddress> AddTorListener(
        const KeyChain& keyChain,
        const std::shared_ptr<ITorProcess>& pTorProcess,
        const uint16_t portNumber,
        int addressIndex
    );

    static int StatusListener(mg_connection* pConnection, void*)
    {
        return HTTPUtil::BuildSuccessResponse(pConnection, "SUCCESS! Your wallet listener is working!");
    }

    std::shared_ptr<ITorProcess> m_pTorProcess;
    RPCServerPtr m_pRPCServer;
    std::optional<TorAddress> m_torAddress;
};