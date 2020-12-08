#include <API/Wallet/Owner/OwnerServer.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorProcess.h>

// APIs
#include <API/Wallet/Owner/Handlers/CreateWalletHandler.h>
#include <API/Wallet/Owner/Handlers/RestoreWalletHandler.h>
#include <API/Wallet/Owner/Handlers/LoginHandler.h>
#include <API/Wallet/Owner/Handlers/LogoutHandler.h>
#include <API/Wallet/Owner/Handlers/SendHandler.h>
#include <API/Wallet/Owner/Handlers/ReceiveHandler.h>
#include <API/Wallet/Owner/Handlers/FinalizeHandler.h>
#include <API/Wallet/Owner/Handlers/RetryTorHandler.h>
#include <API/Wallet/Owner/Handlers/DeleteWalletHandler.h>
#include <API/Wallet/Owner/Handlers/GetWalletSeedHandler.h>
#include <API/Wallet/Owner/Handlers/CancelTxHandler.h>
#include <API/Wallet/Owner/Handlers/GetBalanceHandler.h>
#include <API/Wallet/Owner/Handlers/ListTxsHandler.h>
#include <API/Wallet/Owner/Handlers/RepostTxHandler.h>
#include <API/Wallet/Owner/Handlers/EstimateFeeHandler.h>
#include <API/Wallet/Owner/Handlers/ScanForOutputsHandler.h>

OwnerServer::UPtr OwnerServer::Create(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
{
    RPCServerPtr pServer = RPCServer::Create(
        EServerType::LOCAL,
        std::make_optional<uint16_t>((uint16_t)3421), // TODO: Read port from config (Use same port as v1 owner)
        "/v2",
        LoggerAPI::LogFile::WALLET
    );

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
                "wallet_seed": "agree muscle erase plunge grit effort provide electric social decide include whisper tunnel dizzy bean tumble play robot fire verify program solid weasel nuclear",
                "listener_port": 1100,
                "tor_address": ""
            }
        }
    */
    pServer->AddMethod("create_wallet", std::shared_ptr<RPCMethod>((RPCMethod*)new CreateWalletHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "restore_wallet",
            "id": 1,
            "params": {
                "username": "David",
                "password": "P@ssw0rd123!",
                "wallet_seed": "agree muscle erase plunge grit effort provide electric social decide include whisper tunnel dizzy bean tumble play robot fire verify program solid weasel nuclear"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                "listener_port": 1100,
                "tor_address": ""
            }
        }
    */
    pServer->AddMethod("restore_wallet", std::shared_ptr<RPCMethod>((RPCMethod*)new RestoreWalletHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "login",
            "id": 1,
            "params": {
                "username": "David",
                "password": "P@ssw0rd123!"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                "listener_port": 1100,
                "tor_address": ""
            }
        }
    */
    pServer->AddMethod("login", std::shared_ptr<RPCMethod>((RPCMethod*)new LoginHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "logout",
            "id": 1,
            "params": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk="
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {}
        }
    */
    pServer->AddMethod("logout", std::shared_ptr<RPCMethod>((RPCMethod*)new LogoutHandler(pWalletManager)));

    pServer->AddMethod("send", std::shared_ptr<RPCMethod>((RPCMethod*)new SendHandler(pTorProcess, pWalletManager)));
    pServer->AddMethod("receive", std::shared_ptr<RPCMethod>((RPCMethod*)new ReceiveHandler(pWalletManager)));
    pServer->AddMethod("finalize", std::shared_ptr<RPCMethod>((RPCMethod*)new FinalizeHandler(pTorProcess, pWalletManager)));
    pServer->AddMethod("retry_tor", std::shared_ptr<RPCMethod>((RPCMethod*)new RetryTorHandler(pTorProcess, pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "delete_wallet",
            "id": 1,
            "params": {
                "username": "David",
                "password": "P@ssw0rd123!"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "status": "SUCCESS"
            }
        }
    */
    pServer->AddMethod("delete_wallet", std::shared_ptr<RPCMethod>((RPCMethod*)new DeleteWalletHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_wallet_seed",
            "id": 1,
            "params": {
                "username": "David",
                "password": "P@ssw0rd123!"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "wallet_seed": "agree muscle erase plunge grit effort provide electric social decide include whisper tunnel dizzy bean tumble play robot fire verify program solid weasel nuclear"
            }
        }
    */
    pServer->AddMethod("get_wallet_seed", std::shared_ptr<RPCMethod>((RPCMethod*)new GetWalletSeedHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "cancel_tx",
            "id": 1,
            "params": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                "tx_id": 123
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "status": "SUCCESS"
            }
        }
    */
    pServer->AddMethod("cancel_tx", std::shared_ptr<RPCMethod>((RPCMethod*)new CancelTxHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_balance",
            "id": 1,
            "params": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk="
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "total": 20482100000,
                "unconfirmed": 6282100000,
                "immature": 10200000000,
                "locked": 5700000000,
                "spendable": 4000000000
            }
        }
    */
    pServer->AddMethod("get_balance", std::shared_ptr<RPCMethod>((RPCMethod*)new GetBalanceHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "list_txs",
            "id": 1,
            "params": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                "start_range_ms": 1567361400000,
                "end_range_ms": 1575227400000,
                "statuses": ["COINBASE","SENT","RECEIVED","SENT_CANCELED","RECEIVED_CANCELED","SENDING_NOT_FINALIZED","RECEIVING_IN_PROGRESS", "SENDING_FINALIZED"]
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "txs": []
            }
        }
    */
    pServer->AddMethod("list_txs", std::shared_ptr<RPCMethod>((RPCMethod*)new ListTxsHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "repost_tx",
            "id": 1,
            "params": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                "tx_id": 123,
                "method": "STEM"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "status": "SUCCESS"
            }
        }
    */
    pServer->AddMethod("repost_tx", std::shared_ptr<RPCMethod>((RPCMethod*)new RepostTxHandler(pTorProcess, pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "estimate_fee",
            "id": 1,
            "params": {
                "session_token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
                "amount": 12345678,
                "fee_base": "1000000",
                "change_outputs": 1,
                "selection_strategy": {
                    "strategy": "SMALLEST"
                }
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "fee": "7000000",
                "inputs": [
                    {
                        "keychain_path": "m/1/0/1000",
                        "commitment": "0808657d5346f4061e5484b6f57ed036ce2cb4430599cec5dcb999d07755772010",
                        "amount": 30000000,
                        "status": "Spendable"
                    }
                ]
            }
        }
    */
    pServer->AddMethod("estimate_fee", std::shared_ptr<RPCMethod>((RPCMethod*)new EstimateFeeHandler(pWalletManager)));

    pServer->AddMethod("scan_for_outputs", std::shared_ptr<RPCMethod>((RPCMethod*)new ScanForOutputsHandler(pWalletManager)));

    // TODO: Add the following APIs: 
    // authenticate - Simply validates the password - useful for confirming password before sending funds
    // tx_info - Detailed info about a specific transaction (status, kernels, inputs, outputs, payment proofs, labels, etc)
    // update_labels - Add or remove labels - useful for coin control
    // verify_payment_proof - Takes in an existing payment proof and validates it

    return std::make_unique<OwnerServer>(pServer);
}