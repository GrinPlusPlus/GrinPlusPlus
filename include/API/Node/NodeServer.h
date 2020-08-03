#pragma once

#include <Common/Logger.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <API/Node/Handlers/GetHeaderHandler.h>
#include <API/Node/Handlers/GetBlockHandler.h>
#include <API/Node/Handlers/GetVersionHandler.h>
#include <API/Node/Handlers/GetTipHandler.h>
#include <API/Node/Handlers/PushTransactionHandler.h>

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

    static NodeServer::UPtr Create(const ServerPtr& pServer, const IBlockChainServerPtr& pBlockChain, const IP2PServerPtr& pP2PServer)
    {
        RPCServer::Ptr pForeignServer = RPCServer::Create(pServer, "/v2/foreign", LoggerAPI::LogFile::NODE);
        pForeignServer->AddMethod("get_header", std::make_shared<GetHeaderHandler>(pBlockChain));
        pForeignServer->AddMethod("get_block", std::make_shared<GetBlockHandler>(pBlockChain));
        pForeignServer->AddMethod("get_version", std::make_shared<GetVersionHandler>(pBlockChain));
        pForeignServer->AddMethod("get_tip", std::make_shared<GetTipHandler>(pBlockChain));
        pForeignServer->AddMethod("push_transaction", std::make_shared<PushTransactionHandler>(pBlockChain, pP2PServer));

        RPCServer::Ptr pOwnerServer = RPCServer::Create(pServer, "/v2/owner", LoggerAPI::LogFile::NODE);

        return std::make_unique<NodeServer>(pForeignServer, pOwnerServer);
    }

private:
    RPCServer::Ptr m_pForeignServer;
    RPCServer::Ptr m_pOwnerServer;
};