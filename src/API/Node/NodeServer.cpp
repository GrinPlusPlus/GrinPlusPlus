#include <API/Node/NodeServer.h>

#include "Handlers/GetConfigHandler.h"
#include "Handlers/UpdateConfigHandler.h"
#include "Handlers/GetHeaderHandler.h"
#include "Handlers/GetBlockHandler.h"
#include "Handlers/GetVersionHandler.h"
#include "Handlers/GetTipHandler.h"
#include "Handlers/PushTransactionHandler.h"
#include "Handlers/GetKernelHandler.h"
#include "Handlers/GetUnconfirmedTransactions.h"
#include "Handlers/GetUnspentOutputsHandler.h"
#include "Handlers/GetStatusHandler.h"
#include "Handlers/GetPeersHandler.h"
#include "Handlers/BanPeerHandler.h"
#include "Handlers/UnbanPeerHandler.h"
#include "Handlers/GetConnectedPeersHandler.h"
#include "Handlers/GetTxHashsetHandler.h"
#include "Handlers/GetOutputsHandler.h"


NodeServer::UPtr NodeServer::Create(const ServerPtr& pServer, const IBlockChain::Ptr& pBlockChain, const IP2PServerPtr& pP2PServer)
{
    RPCServer::Ptr pForeignServer = RPCServer::Create(pServer, "/v2/foreign", LoggerAPI::LogFile::NODE);

    pForeignServer->AddMethod("get_header", std::make_shared<GetHeaderHandler>(pBlockChain));
    pForeignServer->AddMethod("get_block", std::make_shared<GetBlockHandler>(pBlockChain));
    pForeignServer->AddMethod("get_kernel", std::make_shared<GetKernelHandler>(pBlockChain));
    pForeignServer->AddMethod("get_unconfirmed_transactions", std::make_shared<GetUnconfirmedTransactions>());
    pForeignServer->AddMethod("get_unspent_outputs", std::make_shared<GetUnspentOutputsHandler>());
    pForeignServer->AddMethod("get_version", std::make_shared<GetVersionHandler>());
    pForeignServer->AddMethod("get_tip", std::make_shared<GetTipHandler>(pBlockChain));
    pForeignServer->AddMethod("push_transaction", std::make_shared<PushTransactionHandler>(pBlockChain, pP2PServer));
    pForeignServer->AddMethod("get_config", std::shared_ptr<RPCMethod>(new GetConfigHandler())); /* DEPRECATED since v1.2.9 */
    pForeignServer->AddMethod("update_config", std::shared_ptr<RPCMethod>(new UpdateConfigHandler())); /* DEPRECATED since v1.2.9 */

    RPCServer::Ptr pOwnerServer = RPCServer::Create(pServer, "/v2/owner", LoggerAPI::LogFile::NODE);
 
    pOwnerServer->AddMethod("get_config", std::shared_ptr<RPCMethod>(new GetConfigHandler()));
    pOwnerServer->AddMethod("update_config", std::shared_ptr<RPCMethod>(new UpdateConfigHandler()));
    pOwnerServer->AddMethod("get_status", std::shared_ptr<RPCMethod>(new GetStatusHandler()));
    pOwnerServer->AddMethod("get_peers", std::shared_ptr<RPCMethod>(new GetPeersHandler()));
    pOwnerServer->AddMethod("ban_peer", std::shared_ptr<RPCMethod>(new BanPeerHandler()));
    pOwnerServer->AddMethod("unban_peer", std::shared_ptr<RPCMethod>(new UnbanPeerHandler()));
    pOwnerServer->AddMethod("get_connected_peers", std::shared_ptr<RPCMethod>(new GetConnectedPeersHandler()));
    pOwnerServer->AddMethod("get_txhashset", std::shared_ptr<RPCMethod>(new GetTxHashsetHandler(pBlockChain)));
    pOwnerServer->AddMethod("get_outputs", std::shared_ptr<RPCMethod>(new GetOutputsHandler(pBlockChain)));

    return std::make_unique<NodeServer>(pForeignServer, pOwnerServer);
}