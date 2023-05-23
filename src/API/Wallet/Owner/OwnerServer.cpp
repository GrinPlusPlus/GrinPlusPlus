#include <API/Wallet/Owner/OwnerServer.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorProcess.h>

// APIs
#include "Handlers/CreateWalletHandler.h"
#include "Handlers/RestoreWalletHandler.h"
#include "Handlers/LoginHandler.h"
#include "Handlers/LogoutHandler.h"
#include "Handlers/SendHandler.h"
#include "Handlers/ReceiveHandler.h"
#include "Handlers/FinalizeHandler.h"
#include "Handlers/RetryTorHandler.h"
#include "Handlers/DeleteWalletHandler.h"
#include "Handlers/GetWalletSeedHandler.h"
#include "Handlers/CancelTxHandler.h"
#include "Handlers/GetBalanceHandler.h"
#include "Handlers/ListTxsHandler.h"
#include "Handlers/ListOutputsHandler.h"
#include "Handlers/RepostTxHandler.h"
#include "Handlers/EstimateFeeHandler.h"
#include "Handlers/ScanForOutputsHandler.h"
#include "Handlers/AddressInfoHandler.h"
#include "Handlers/ListWalletsHandler.h"
#include "Handlers/UpdateTorConfigHandler.h"
#include "Handlers/GetTorConfigHandler.h"
#include "Handlers/GetWalletAddressHandler.h"
#include "Handlers/GetAddressSecretKeyHandler.h"
#include "Handlers/GetNewWalletAddressHandler.h"
#include "Handlers/CreateSlatepackHandler.h"
#include "Handlers/DecodeSlatepackHandler.h"
#include "Handlers/InitSendTxHandler.h"
#include "Handlers/AuthenticateWalletHandler.h"
#include "Handlers/ChangePasswordHandler.h"
#include "Handlers/GetTopLevelDirectoryHandler.h"
#include "Handlers/NodeHeightHandler.h"
#include "Handlers/GetStoredTxHandler.h"

OwnerServer::UPtr OwnerServer::Create(const uint16_t& serverPort,
                                      const TorProcess::Ptr& pTorProcess,
                                      const IWalletManagerPtr& pWalletManager)
{
    RPCServerPtr pOwnerServer = RPCServer::Create(
        EServerType::LOCAL,
        std::make_optional<uint16_t>(serverPort),
        "/v3/owner",
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
    pOwnerServer->AddMethod("create_wallet", std::shared_ptr<RPCMethod>(new CreateWalletHandler(pWalletManager, pTorProcess)));

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
    pOwnerServer->AddMethod("restore_wallet", std::shared_ptr<RPCMethod>(new RestoreWalletHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "open_wallet",
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
    pOwnerServer->AddMethod("login", std::shared_ptr<RPCMethod>(new LoginHandler(pWalletManager, pTorProcess)));
    pOwnerServer->AddMethod("open_wallet", std::shared_ptr<RPCMethod>(new LoginHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "close_wallet",
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
    pOwnerServer->AddMethod("logout", std::shared_ptr<RPCMethod>(new LogoutHandler(pWalletManager)));
    pOwnerServer->AddMethod("close_wallet", std::shared_ptr<RPCMethod>(new LogoutHandler(pWalletManager)));

    pOwnerServer->AddMethod("send", std::shared_ptr<RPCMethod>(new SendHandler(pTorProcess, pWalletManager)));
    pOwnerServer->AddMethod("receive", std::shared_ptr<RPCMethod>(new ReceiveHandler(pWalletManager)));

    pOwnerServer->AddMethod("finalize", std::shared_ptr<RPCMethod>(new FinalizeHandler(pTorProcess, pWalletManager)));
    pOwnerServer->AddMethod("finalize_tx", std::shared_ptr<RPCMethod>(new FinalizeHandler(pTorProcess, pWalletManager)));

    pOwnerServer->AddMethod("retry_tor", std::shared_ptr<RPCMethod>(new RetryTorHandler(pTorProcess, pWalletManager)));

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
    pOwnerServer->AddMethod("delete_wallet", std::shared_ptr<RPCMethod>(new DeleteWalletHandler(pWalletManager)));

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
    pOwnerServer->AddMethod("get_wallet_seed", std::shared_ptr<RPCMethod>(new GetWalletSeedHandler(pWalletManager)));
    pOwnerServer->AddMethod("get_mnemonic", std::shared_ptr<RPCMethod>(new GetWalletSeedHandler(pWalletManager)));

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
    pOwnerServer->AddMethod("cancel_tx", std::shared_ptr<RPCMethod>(new CancelTxHandler(pWalletManager)));

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
    pOwnerServer->AddMethod("get_balance", std::shared_ptr<RPCMethod>(new GetBalanceHandler(pWalletManager)));
    pOwnerServer->AddMethod("retrieve_summary_info", std::shared_ptr<RPCMethod>(new GetBalanceHandler(pWalletManager)));

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
    pOwnerServer->AddMethod("list_txs", std::shared_ptr<RPCMethod>(new ListTxsHandler(pWalletManager)));
    pOwnerServer->AddMethod("retrieve_txs", std::shared_ptr<RPCMethod>(new ListTxsHandler(pWalletManager)));

    pOwnerServer->AddMethod("list_outputs", std::shared_ptr<RPCMethod>(new ListOutputsHandler(pWalletManager)));
    pOwnerServer->AddMethod("retrieve_outputs", std::shared_ptr<RPCMethod>(new ListOutputsHandler(pWalletManager)));

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
    pOwnerServer->AddMethod("repost_tx", std::shared_ptr<RPCMethod>(new RepostTxHandler(pTorProcess, pWalletManager)));
    pOwnerServer->AddMethod("post_tx", std::shared_ptr<RPCMethod>(new RepostTxHandler(pTorProcess, pWalletManager)));

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
    pOwnerServer->AddMethod("estimate_fee", std::shared_ptr<RPCMethod>(new EstimateFeeHandler(pWalletManager)));

    pOwnerServer->AddMethod("scan_for_outputs", std::shared_ptr<RPCMethod>(new ScanForOutputsHandler(pWalletManager)));

    pOwnerServer->AddMethod("address_info", std::shared_ptr<RPCMethod>(new AddressInfoHandler()));

    pOwnerServer->AddMethod("list_wallets", std::shared_ptr<RPCMethod>(new ListWalletsHandler(pWalletManager)));

    pOwnerServer->AddMethod("set_tor_config", std::shared_ptr<RPCMethod>(new UpdateTorConfigHandler(pTorProcess)));
    
    pOwnerServer->AddMethod("get_tor_config", std::shared_ptr<RPCMethod>(new GetTorConfigHandler()));

    pOwnerServer->AddMethod("get_slatepack_address", std::shared_ptr<RPCMethod>(new GetWalletAddressHandler(pWalletManager)));

    pOwnerServer->AddMethod("get_slatepack_secret_key", std::shared_ptr<RPCMethod>(new GetAddressSecretKeyHandler(pWalletManager)));

    pOwnerServer->AddMethod("get_new_slatepack_address", std::shared_ptr<RPCMethod>(new GetNewWalletAddressHandler(pWalletManager, pTorProcess)));

    // TODO: Add the following APIs: 
    // update_labels - Add or remove labels - useful for coin control
    // verify_payment_proof - Takes in an existing payment proof and validates it

    pOwnerServer->AddMethod("create_slatepack_message", std::shared_ptr<RPCMethod>(new CreateSlatepackHandler(pWalletManager)));

    pOwnerServer->AddMethod("decode_slatepack_message", std::shared_ptr<RPCMethod>(new DecodeSlatepackHandler(pWalletManager)));

    pOwnerServer->AddMethod("init_send_tx", std::shared_ptr<RPCMethod>(new InitSendTxHandler(pTorProcess, pWalletManager)));

    pOwnerServer->AddMethod("authenticate", std::shared_ptr<RPCMethod>(new AuthenticateWalletHandler(pWalletManager)));

    pOwnerServer->AddMethod("change_password", std::shared_ptr<RPCMethod>(new ChangePasswordHandler(pWalletManager)));

    pOwnerServer->AddMethod("get_top_level_directory", std::shared_ptr<RPCMethod>(new GetTopLevelDirectoryHandler(pWalletManager)));

    pOwnerServer->AddMethod("node_height", std::shared_ptr<RPCMethod>(new NodeHeightHandler(pWalletManager)));

    pOwnerServer->AddMethod("get_stored_tx", std::shared_ptr<RPCMethod>(new GetStoredTxHandler(pWalletManager)));

    return std::make_unique<OwnerServer>(pOwnerServer);
}