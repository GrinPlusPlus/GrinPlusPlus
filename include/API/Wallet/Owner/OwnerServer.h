#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <API/Wallet/Owner/Handlers/CreateWalletHandler.h>
#include <API/Wallet/Owner/Handlers/SendHandler.h>
#include <API/Wallet/Owner/Handlers/ReceiveHandler.h>
#include <API/Wallet/Owner/Handlers/FinalizeHandler.h>
#include <API/Wallet/Owner/Handlers/RetryTorHandler.h>

class OwnerServer
{
public:
    using Ptr = std::shared_ptr<OwnerServer>;

    OwnerServer::OwnerServer(const RPCServerPtr& pServer) : m_pServer(pServer) { }

    // TODO: Add e2e encryption
    static OwnerServer::Ptr OwnerServer::Create(const Config& config, const IWalletManagerPtr& pWalletManager)
    {
        RPCServerPtr pServer = RPCServer::Create(EServerType::LOCAL, std::make_optional<uint16_t>((uint16_t)3421), "/v2"); // TODO: Read port from config (Use same port as v1 owner)

        /*
            Request:
            {
                "jsonrpc": "2.0",
                "method": "create_wallet",
                "id": 1,
                "params": {
                    "username": "David",
                    "password": "P@ssw0rd123!",
                    "num_seed_words": 24
                }
            }

            Reply:
            {
                "id": 1,
                "jsonrpc": "2.0",
                "result": {
                    "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                    "wallet_seed": "agree muscle erase plunge grit effort provide electric social decide include whisper tunnel dizzy bean tumble play robot fire verify program solid weasel nuclear"
                }
            }
        */
        pServer->AddMethod("create_wallet", std::shared_ptr<RPCMethod>((RPCMethod*)new CreateWalletHandler(pWalletManager)));

        pServer->AddMethod("send", std::shared_ptr<RPCMethod>((RPCMethod*)new SendHandler(config, pWalletManager)));
        pServer->AddMethod("receive", std::shared_ptr<RPCMethod>((RPCMethod*)new ReceiveHandler(pWalletManager)));
        pServer->AddMethod("finalize", std::shared_ptr<RPCMethod>((RPCMethod*)new FinalizeHandler(pWalletManager)));
        pServer->AddMethod("retry_tor", std::shared_ptr<RPCMethod>((RPCMethod*)new RetryTorHandler(config, pWalletManager)));

        // TODO: Add the following APIs: 
        // login - Login as an existing user
        // restore_wallet - Restores wallet by seed
        // authenticate - Simply validates the password - useful for confirming password before sending funds
        // cancel_tx - Cancels a transaction by Id or UUID
        // repost_tx - Reposts a transaction that was already finalized but never confirmed on chain
        // list_txs - retrieves basic info about all transactions with optional filter for timerange, type, etc.
        // tx_info - Detailed info about a specific transaction (status, kernels, inputs, outputs, payment proofs, labels, etc)
        // update_labels - Add or remove labels - useful for coin control
        // verify_payment_proof - Takes in an existing payment proof and validates it
        // get_seed_phrase - Returns the user's seed - requires a password

        return std::shared_ptr<OwnerServer>(new OwnerServer(pServer));
    }

private:
    RPCServerPtr m_pServer;
};