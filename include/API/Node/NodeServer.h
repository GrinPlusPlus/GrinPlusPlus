#pragma once

#include <Common/Logger.h>
#include <BlockChain/BlockChain.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <P2P/P2PServer.h>

class NodeServer
{
public:
    using UPtr = std::unique_ptr<NodeServer>;

    NodeServer(const RPCServer::Ptr& pForeignServer, const RPCServer::Ptr& pOwnerServer)
        : m_pForeignServer(pForeignServer), m_pOwnerServer(pOwnerServer)
    {
        LOG_INFO("Starting node server");
    }

    ~NodeServer()
    {
        LOG_INFO("Shutting down node server");
    }

    static NodeServer::UPtr Create(
        const ServerPtr& pServer,
        const IBlockChain::Ptr& pBlockChain,
        const IP2PServerPtr& pP2PServer
    );

private:
    RPCServer::Ptr m_pForeignServer;
    RPCServer::Ptr m_pOwnerServer;
};