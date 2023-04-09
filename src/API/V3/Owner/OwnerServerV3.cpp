#include <API/V3/Owner/OwnerServerV3.h>
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
#include "Handlers/GetNewWalletAddressHandler.h"

OwnerServerV3::UPtr OwnerServerV3::Create(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
{
    RPCServerPtr pServer = RPCServer::Create(
        EServerType::LOCAL,
        std::make_optional<uint16_t>((uint16_t)3420), // TODO: Read port from config (Use same port as v1 owner)
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
                "name": "David",
                "password": "P@ssw0rd123!",
                "mnemonic_length": 24
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": null
            }
        }
    */
    pServer->AddMethod("create_wallet", std::shared_ptr<RPCMethod>(new CreateWalletHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "restore_wallet",
            "id": 1,
            "params": {
                "name": "David",
                "password": "P@ssw0rd123!",
                "mnemonic": "agree muscle erase plunge grit effort provide electric social decide include whisper tunnel dizzy bean tumble play robot fire verify program solid weasel nuclear"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": null
            }
        }
    */
    pServer->AddMethod("restore_wallet", std::shared_ptr<RPCMethod>(new RestoreWalletHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "open_wallet",
            "id": 1,
            "params": {
                "name": "David",
                "password": "P@ssw0rd123!"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
            }
        }
    */
    pServer->AddMethod("open_wallet", std::shared_ptr<RPCMethod>(new LoginHandler(pWalletManager, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "close_wallet",
            "id": 1,
            "params": {
                "token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk="
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": null
            }
        }
    */
    pServer->AddMethod("close_wallet", std::shared_ptr<RPCMethod>(new LogoutHandler(pWalletManager)));

    pServer->AddMethod("send", std::shared_ptr<RPCMethod>(new SendHandler(pTorProcess, pWalletManager)));
    pServer->AddMethod("receive", std::shared_ptr<RPCMethod>(new ReceiveHandler(pWalletManager)));
    pServer->AddMethod("finalize_tx", std::shared_ptr<RPCMethod>(new FinalizeHandler(pTorProcess, pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "delete_wallet",
            "id": 1,
            "params": {
                "name": "David",
                "password": "P@ssw0rd123!"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": ""
            }
        }
    */
    pServer->AddMethod("delete_wallet", std::shared_ptr<RPCMethod>(new DeleteWalletHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_mnemonic",
            "id": 1,
            "params": {
                "name": "David",
                "password": "P@ssw0rd123!"
            }
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok: "wallet_seed": "agree muscle erase plunge grit effort provide electric social decide include whisper tunnel dizzy bean tumble play robot fire verify program solid weasel nuclear"
            }
        }
    */
    pServer->AddMethod("get_mnemonic", std::shared_ptr<RPCMethod>(new GetWalletSeedHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "cancel_tx",
            "id": 1,
            "params": {
                "token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
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
    pServer->AddMethod("cancel_tx", std::shared_ptr<RPCMethod>(new CancelTxHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_balance",
            "id": 1,
            "params": {
                "token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk="
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
    pServer->AddMethod("get_balance", std::shared_ptr<RPCMethod>(new GetBalanceHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "retrieve_txs",
            "id": 1,
            "params": {
                "token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
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
    pServer->AddMethod("retrieve_txs", std::shared_ptr<RPCMethod>(new ListTxsHandler(pWalletManager)));

    pServer->AddMethod("list_outputs", std::shared_ptr<RPCMethod>(new ListOutputsHandler(pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "repost_tx",
            "id": 1,
            "params": {
                "token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
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
    pServer->AddMethod("post_tx", std::shared_ptr<RPCMethod>(new RepostTxHandler(pTorProcess, pWalletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "estimate_fee",
            "id": 1,
            "params": {
                "token": "mFHve+/CFsPuQf1+Anp24+R1rLZCVBIyKF+fJEuxAappgT2WKMfpOiNwvRk=",
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
    pServer->AddMethod("estimate_fee", std::shared_ptr<RPCMethod>(new EstimateFeeHandler(pWalletManager)));

    pServer->AddMethod("scan_for_outputs", std::shared_ptr<RPCMethod>(new ScanForOutputsHandler(pWalletManager)));

    pServer->AddMethod("address_info", std::shared_ptr<RPCMethod>(new AddressInfoHandler()));

    pServer->AddMethod("list_wallets", std::shared_ptr<RPCMethod>(new ListWalletsHandler(pWalletManager)));

    pServer->AddMethod("set_tor_config", std::shared_ptr<RPCMethod>(new UpdateTorConfigHandler(pTorProcess)));
    
    pServer->AddMethod("get_tor_config", std::shared_ptr<RPCMethod>(new GetTorConfigHandler()));

    pServer->AddMethod("get_slatepack_address", std::shared_ptr<RPCMethod>(new GetWalletAddressHandler(pWalletManager)));

    pServer->AddMethod("get_new_slatepack_address", std::shared_ptr<RPCMethod>(new GetNewWalletAddressHandler(pWalletManager, pTorProcess)));

    // TODO: Add the following APIs: 
    // authenticate - Simply validates the password - useful for confirming password before sending funds
    // tx_info - Detailed info about a specific transaction (status, kernels, inputs, outputs, payment proofs, labels, etc)
    // update_labels - Add or remove labels - useful for coin control
    // verify_payment_proof - Takes in an existing payment proof and validates it

    return std::make_unique<OwnerServerV3>(pServer);
}