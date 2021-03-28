#pragma once

#include <Net/Servers/RPC/RPCServer.h>

// Forward Declarations
class ITorProcess;
class IWalletManager;

class OwnerServer
{
public:
    using UPtr = std::unique_ptr<OwnerServer>;

    OwnerServer(const RPCServerPtr& pServer) : m_pServer(pServer) { }
    ~OwnerServer() { LOG_INFO("Shutting down owner API"); }

    // TODO: Add e2e encryption
    static OwnerServer::UPtr Create(
        const std::shared_ptr<ITorProcess>& pTorProcess,
        const std::shared_ptr<IWalletManager>& pWalletManager
    );

private:
    RPCServerPtr m_pServer;
};