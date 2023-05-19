#include <API/Node/NodeServer.h>

#include "Handlers/GetBlockHandler.h"
#include "Handlers/GetHeaderHandler.h"
#include "Handlers/GetKernelHandler.h"
#include "Handlers/GetOutputsHandler.h"
#include "Handlers/GetTipHandler.h"
#include "Handlers/GetUnspentOutputsHandler.h"
#include "Handlers/GetVersionHandler.h"
#include "Handlers/PushTransactionHandler.h"

#include "Handlers/BanPeerHandler.h"
#include "Handlers/GetConfigHandler.h"
#include "Handlers/GetConnectedPeersHandler.h"
#include "Handlers/GetPeersHandler.h"
#include "Handlers/GetStatusHandler.h"
#include "Handlers/UnbanPeerHandler.h"
#include "Handlers/UpdateConfigHandler.h"

NodeServer::UPtr NodeServer::Create(const ServerPtr& pServer, const IBlockChain::Ptr& pBlockChain,
                                    const IP2PServerPtr& pP2PServer, const ITxHashSetPtr& pTxHashSet,
                                    const IDatabasePtr& pDatabase)
{
    RPCServer::Ptr pForeignServer = RPCServer::Create(pServer, "/v2/foreign", LoggerAPI::LogFile::NODE);
    pForeignServer->AddMethod("get_block", std::make_shared<GetBlockHandler>(pBlockChain));
    pForeignServer->AddMethod("get_header", std::make_shared<GetHeaderHandler>(pBlockChain));
    pForeignServer->AddMethod("get_kernel", std::make_shared<GetKernelHandler>(pBlockChain));
    pForeignServer->AddMethod("get_outputs", std::shared_ptr<RPCMethod>(new GetOutputsHandler(pTxHashSet, pDatabase)));
    pForeignServer->AddMethod("get_tip", std::make_shared<GetTipHandler>(pBlockChain));
    pForeignServer->AddMethod("get_unspent_outputs", std::make_shared<GetUnspentOutputsHandler>(pTxHashSet, pDatabase));
    pForeignServer->AddMethod("get_version", std::make_shared<GetVersionHandler>());
    pForeignServer->AddMethod("push_transaction", std::make_shared<PushTransactionHandler>(pBlockChain, pP2PServer));
    
    RPCServer::Ptr pOwnerServer = RPCServer::Create(pServer, "/v2/owner", LoggerAPI::LogFile::NODE);
    pOwnerServer->AddMethod("ban_peer", std::shared_ptr<RPCMethod>(new BanPeerHandler(pP2PServer)));
    pOwnerServer->AddMethod("get_config", std::shared_ptr<RPCMethod>(new GetConfigHandler()));
    pOwnerServer->AddMethod("get_connected_peers", std::shared_ptr<RPCMethod>(new GetConnectedPeersHandler(pP2PServer)));
    pOwnerServer->AddMethod("get_peers", std::shared_ptr<RPCMethod>(new GetPeersHandler(pP2PServer)));
    pOwnerServer->AddMethod("get_status", std::shared_ptr<RPCMethod>(new GetStatusHandler(pBlockChain, pP2PServer)));
    pOwnerServer->AddMethod("unban_peer", std::shared_ptr<RPCMethod>(new UnbanPeerHandler(pP2PServer)));
    pOwnerServer->AddMethod("update_config", std::shared_ptr<RPCMethod>(new UpdateConfigHandler()));

    return std::make_unique<NodeServer>(pForeignServer, pOwnerServer);
}