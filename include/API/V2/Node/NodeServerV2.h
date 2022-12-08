#pragma once

#include <Net/Servers/RPC/RPCServer.h>

// Forward Declarations
class ITorProcess;
class IWalletManager;

class NodeServerV2
{
public:
    using UPtr = std::unique_ptr<NodeServerV2>;

    NodeServerV2(const RPCServerPtr& pServer) : m_pServer(pServer) { }
    ~NodeServerV2() { LOG_INFO("Shutting down node RPC API"); }
    
    static NodeServerV2::UPtr Create();

private:
    RPCServerPtr m_pServer;
};