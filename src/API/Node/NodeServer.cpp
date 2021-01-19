#include <API/Node/NodeServer.h>

#include "Handlers/GetConfigHandler.h"
#include "Handlers/UpdateConfigHandler.h"
#include "Handlers/GetHeaderHandler.h"
#include "Handlers/GetBlockHandler.h"
#include "Handlers/GetVersionHandler.h"
#include "Handlers/GetTipHandler.h"
#include "Handlers/PushTransactionHandler.h"

NodeServer::UPtr NodeServer::Create(const ServerPtr& pServer, const IBlockChain::Ptr& pBlockChain, const IP2PServerPtr& pP2PServer)
{
    RPCServer::Ptr pForeignServer = RPCServer::Create(pServer, "/v2/foreign", LoggerAPI::LogFile::NODE);
    pForeignServer->AddMethod("get_header", std::make_shared<GetHeaderHandler>(pBlockChain));
    pForeignServer->AddMethod("get_block", std::make_shared<GetBlockHandler>(pBlockChain));
    pForeignServer->AddMethod("get_version", std::make_shared<GetVersionHandler>(pBlockChain));
    pForeignServer->AddMethod("get_tip", std::make_shared<GetTipHandler>(pBlockChain));
    pForeignServer->AddMethod("push_transaction", std::make_shared<PushTransactionHandler>(pBlockChain, pP2PServer));

    pForeignServer->AddMethod("get_config", std::make_shared<GetConfigHandler>());
    pForeignServer->AddMethod("update_config", std::make_shared<UpdateConfigHandler>());

    RPCServer::Ptr pOwnerServer = RPCServer::Create(pServer, "/v2/owner", LoggerAPI::LogFile::NODE);

    return std::make_unique<NodeServer>(pForeignServer, pOwnerServer);
}