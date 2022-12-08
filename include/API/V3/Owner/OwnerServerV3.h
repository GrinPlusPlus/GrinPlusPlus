#pragma once

#include <Net/Servers/RPC/RPCServer.h>

// Forward Declarations
class ITorProcess;
class IWalletManager;

class OwnerServerV3
{
public:
    using UPtr = std::unique_ptr<OwnerServerV3>;

    OwnerServerV3(const RPCServerPtr& pServer) : m_pServer(pServer) { }
    ~OwnerServerV3() { LOG_INFO("Shutting down owner RPC API"); }

    // TODO: Add e2e encryption
    static OwnerServerV3::UPtr Create(
        const std::shared_ptr<ITorProcess>& pTorProcess,
        const std::shared_ptr<IWalletManager>& pWalletManager
    );

private:
    RPCServerPtr m_pServer;
};