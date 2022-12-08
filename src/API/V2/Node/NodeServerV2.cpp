#include <API/V2/Node/NodeServerV2.h>

// APIs
#include "Handlers/GetVersionHandler.h"
#include "Handlers/GetPeersHandler.h"
#include "Handlers/GetConnectedPeersHandler.h"
#include "Handlers/BanPeerHandler.h"
#include "Handlers/UnbanPeerHandler.h"


NodeServerV2::UPtr NodeServerV2::Create()
{
    RPCServerPtr pServer = RPCServer::Create(
        EServerType::LOCAL,
        std::make_optional<uint16_t>((uint16_t)3413),
        "/v2/owner",
        LoggerAPI::LogFile::NODE
    );

    
    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_version",
            "id": 1,
            "params": []
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": {
                    "node_version": "2.1.0-beta.2",
                    "block_header_version": 2
                }
            }
        }
    */
    pServer->AddMethod("get_version", std::shared_ptr<RPCMethod>(new GetVersionHandler()));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_peers",
            "id": 1,
            "params": ["70.50.33.130:3414"]
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": [
                    {
                        "addr": "70.50.33.130:3414",
                        "ban_reason": "None",
                        "capabilities": {
                            "bits": 15
                        },
                        "flags": "Defunct",
                        "last_banned": 0,
                        "last_connected": 1570129317,
                        "user_agent": "MW/Grin 2.0.0"
                    }
                ]
            }
        }
    */
    pServer->AddMethod("get_peers", std::shared_ptr<RPCMethod>(new GetPeersHandler()));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "get_connected_peers",
            "id": 1,
            "params": []
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": [
                    {
                        "addr": "35.176.195.242:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "47.97.198.21:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "148.251.16.13:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "68.195.18.155:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "52.53.221.15:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 0,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "109.74.202.16:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "121.43.183.180:3414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    },
                    {
                        "addr": "35.157.247.209:23414",
                        "capabilities": {
                        "bits": 15
                        },
                        "direction": "Outbound",
                        "height": 374510,
                        "total_difficulty": 1133954621205750,
                        "user_agent": "MW/Grin 2.0.0",
                        "version": 1
                    }
                ]
            }
        }
    */
    pServer->AddMethod("get_peers", std::shared_ptr<RPCMethod>(new GetConnectedPeersHandler()));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "ban_peer",
            "id": 1,
            "params": ["70.50.33.130:3414"]
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
    pServer->AddMethod("ban_peer", std::shared_ptr<RPCMethod>(new BanPeerHandler()));

     /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "unban_peer",
            "id": 1,
            "params": ["70.50.33.130:3414"]
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
    pServer->AddMethod("unban_peer", std::shared_ptr<RPCMethod>(new UnbanPeerHandler()));

    return std::make_unique<NodeServerV2>(pServer);
}