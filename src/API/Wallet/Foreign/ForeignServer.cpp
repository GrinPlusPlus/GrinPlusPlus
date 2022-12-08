#include <API/Wallet/Foreign/ForeignServer.h>

#include <Wallet/WalletManager.h>
#include <Wallet/Keychain/KeyChain.h>
#include "Handlers/ReceiveTxHandler.h"
#include "Handlers/FinalizeTxHandler.h"
#include "Handlers/CheckVersionHandler.h"
#include "Handlers/BuildCoinbaseHandler.h"

ForeignServer::UPtr ForeignServer::Create(
    const KeyChain& keyChain,
    const std::shared_ptr<ITorProcess>& pTorProcess,
    IWalletManager& walletManager,
    const SessionToken& token,
    const int currentAddressIndex)
{
    RPCServerPtr pServer = RPCServer::Create(
        EServerType::PUBLIC,
        std::nullopt,
        "/v2/foreign",
        LoggerAPI::LogFile::WALLET
    );

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "check_version",
            "id": 1,
            "params": []
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": {
                    "foreign_api_version": 2,
                    "supported_slate_versions": [
                        "V4"
                    ]
                }
            }
        }
    */
    pServer->AddMethod("check_version", std::shared_ptr<RPCMethod>((RPCMethod*)new CheckVersionHandler()));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "build_coinbase",
            "id": 1,
            "params": [
                {
                    "fees": 0,
                    "height": 0,
                    "key_id": null
                }
            ]
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": {
                    "kernel": {
                        "excess": "08dfe86d732f2dd24bac36aa7502685221369514197c26d33fac03041d47e4b490",
                        "excess_sig": "8f07ddd5e9f5179cff19486034181ed76505baaad53e5d994064127b56c5841be02fa098c54c9bf638e0ee1ad5eb896caa11565f632be7b9cd65643ba371044f",
                        "features": "Coinbase",
                        "fee": "0",
                        "lock_height": "0"
                    },
                    "key_id": "0300000000000000000000000400000000",
                    "output": {
                        "commit": "08fe198e525a5937d0c5d01fa354394d2679be6df5d42064a0f7550c332fce3d9d",
                        "features": "Coinbase",
                        "proof": "9d8488fcb43c9c0f683b9ce62f3c8e047b71f2b4cd94b99a3c9a36aef3bb8361ee17b4489eb5f6d6507250532911acb76f18664604c2ca4215347a5d5d8e417d00ca2d59ec29371286986428b0ec1177fc2e416339ea8542eff8186550ad0d65ffac35d761c38819601d331fd427576e2fff823bbc3faa04f49f5332bd4de46cd4f83d0fd46cdb1dfb87069e95974e4a45e0235db71f5efe5cec83bbb30e152ac50a010ef4e57e33aabbeb894b9114f90bb5c3bb03b009014e358aa3914b1a208eb9d8806fbb679c256d4c1a47b0fce3f1235d58192cb7f615bd7c5dab48486db8962c2a594e69ff70029784a810b4eb76b0516805f3417308cda8acb38b9a3ea061568f0c97f5b46a3beff556dc7ebb58c774f08be472b4b6f603e5f8309c2d1f8d6f52667cb86816b330eca5374148aa898f5bbaf3f23a3ebcdc359ee1e14d73a65596c0ddf51f123234969ac8b557ba9dc53255dd6f5c0d3dd2c035a6d1a1185102612fdca474d018b9f9e81acfa3965d42769f5a303bbaabb78d17e0c026b8be0039c55ad1378c8316101b5206359f89fd1ee239115dde458749a040997be43c039055594cab76f602a0a1ee4f5322f3ab1157342404239adbf8b6786544cd67d9891c2689530e65f2a4b8e52d8551b92ffefb812ffa4a472a10701884151d1fb77d8cdc0b1868cb31b564e98e4c035e0eaa26203b882552c7b69deb0d8ec67cf28d5ec044554f8a91a6cae87eb377d6d906bba6ec94dda24ebfd372727f68334af798b11256d88e17cef7c4fed092128215f992e712ed128db2a9da2f5e8fadea9395bddd294a524dce47f818794c56b03e1253bf0fb9cb8beebc5742e4acf19c24824aa1d41996e839906e24be120a0bdf6800da599ec9ec3d1c4c11571c9f143eadbb554fa3c8c9777994a3f3421d454e4ec54c11b97eea3e4e6ede2d97a2bc"
                    }
                }
            }
        }
    */
    pServer->AddMethod("build_coinbase", std::shared_ptr<RPCMethod>((RPCMethod*)new BuildCoinbaseHandler(walletManager)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "receive_tx",
            "id": 1,
            "params": [
                {
                    "version_info":
                    {
                        "version": 2,
                        "orig_version": 2,
                        "block_header_version": 2
                    },
                    "num_participants": 2,
                    "id": "0436430c-2b02-624c-2032-570501212b00",
                    "tx":
                    {
                        "offset": "d202964900000000d302964900000000d402964900000000d502964900000000",
                        "body": {
                            "inputs": [
                                {
                                    "features": "Coinbase",
                                    "commit": "087df32304c5d4ae8b2af0bc31e700019d722910ef87dd4eec3197b80b207e3045"
                                },
                                {
                                    "features": "Coinbase",
                                    "commit": "08e1da9e6dc4d6e808a718b2f110a991dd775d65ce5ae408a4e1f002a4961aa9e7"
                                }
                            ],
                            "outputs": [
                                {
                                    "features": "Plain",
                                    "commit": "0812276cc788e6870612296d926cba9f0e7b9810670710b5a6e6f1ba006d395774",
                                    "proof": "dcff6175390c602bfa92c2ffd1a9b2d84dcc9ea941f6f317bdd0f875244ef23e696fd17c71df79760ce5ce1a96aab1d15dd057358dc835e972febeb86d50ccec0dad7cfe0246d742eb753cf7b88c045d15bc7123f8cf7155647ccf663fca92a83c9a65d0ed756ea7ebffd2cac90c380a102ed9caaa355d175ed0bf58d3ac2f5e909d6c447dfc6b605e04925c2b17c33ebd1908c965a5541ea5d2ed45a0958e6402f89d7a56df1992e036d836e74017e73ccad5cb3a82b8e139e309792a31b15f3ffd72ed033253428c156c2b9799458a25c1da65b719780a22de7fe7f437ae2fccd22cf7ea357ab5aa66a5ef7d71fb0dc64aa0b5761f68278062bb39bb296c787e4cabc5e2a2933a416ce1c9a9696160386449c437e9120f7bb26e5b0e74d1f2e7d5bcd7aafb2a92b87d1548f1f911fb06af7bd6cc13cee29f7c9cb79021aed18186272af0e9d189ec107c81a8a3aeb4782b0d950e4881aa51b776bb6844b25bce97035b48a9bdb2aea3608687bcdd479d4fa998b5a839ff88558e4a29dff0ed13b55900abb5d439b70793d902ae9ad34587b18c919f6b875c91d14deeb1c373f5e76570d59a6549758f655f1128a54f162dfe8868e1587028e26ad91e528c5ae7ee9335fa58fb59022b5de29d80f0764a9917390d46db899acc6a5b416e25ecc9dccb7153646addcc81cadb5f0078febc7e05d7735aba494f39ef05697bbcc9b47b2ccc79595d75fc13c80678b5e237edce58d731f34c05b1ddcaa649acf2d865bbbc3ceda10508bcdd29d0496744644bf1c3516f6687dfeef5649c7dff90627d642739a59d91a8d1d0c4dc55d74a949e1074427664b467992c9e0f7d3af9d6ea79513e8946ddc0d356bac49878e64e6a95b0a30214214faf2ce317fa622ff3266b32a816e10a18e6d789a5da1f23e67b4f970a68a7bcd9e18825ee274b0483896a40"
                                }
                            ],
                            "kernels": [
                                {
                                    "features": "Plain",
                                    "fee": "7000000",
                                    "lock_height": "0",
                                    "excess": "000000000000000000000000000000000000000000000000000000000000000000",
                                    "excess_sig": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                                }
                            ]
                        }
                    },
                    "amount": "60000000000",
                    "fee": "7000000",
                    "height": "5",
                    "lock_height": "0",
                    "ttl_cutoff_height": null,
                    "payment_proof": null,
                    "participant_data": [
                        {
                            "id": "0",
                            "public_blind_excess": "033ac2158fa0077f087de60c19d8e431753baa5b63b6e1477f05a2a6e7190d4592",
                            "public_nonce": "031b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d5dd078f",
                            "part_sig": null,
                            "message": null,
                            "message_sig": null
                        }
                    ]
                },
                null,
                "Thanks, Yeastplume"
            ]
        }


        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result":
            {
                "Ok":
                {
                    "amount": "60000000000",
                    "fee": "7000000",
                    "height": "5",
                    "id": "0436430c-2b02-624c-2032-570501212b00",
                    "lock_height": "0",
                    "ttl_cutoff_height": null,
                    "payment_proof": null,
                    "num_participants": 2,
                    "participant_data": [
                    {
                        "id": "0",
                        "message": null,
                        "message_sig": null,
                        "part_sig": null,
                        "public_blind_excess": "033ac2158fa0077f087de60c19d8e431753baa5b63b6e1477f05a2a6e7190d4592",
                        "public_nonce": "031b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d5dd078f"
                    },
                    {
                        "id": "1",
                        "message": "Thanks, Yeastplume",
                        "message_sig": "8f07ddd5e9f5179cff19486034181ed76505baaad53e5d994064127b56c5841b30a1f1b21eade1b4bd211e1f137fbdbca1b78dc43da21b1695f6a0edf2437ff9",
                        "part_sig": "8f07ddd5e9f5179cff19486034181ed76505baaad53e5d994064127b56c5841b2b35bd28dfd2269e0670e0cf9270bd6df2d03fbd64523ee4ae622396055b96fc",
                        "public_blind_excess": "038fe0443243dab173c068ef5fa891b242d2b5eb890ea09475e6e381170442ee16",
                        "public_nonce": "031b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d5dd078f"
                    }
                    ],
                    "tx": {
                        "body": {
                            "inputs": [
                            {
                                "commit": "087df32304c5d4ae8b2af0bc31e700019d722910ef87dd4eec3197b80b207e3045",
                                "features": "Coinbase"
                            },
                            {
                                "commit": "08e1da9e6dc4d6e808a718b2f110a991dd775d65ce5ae408a4e1f002a4961aa9e7",
                                "features": "Coinbase"
                            }
                            ],
                            "kernels": [
                            {
                                "excess": "000000000000000000000000000000000000000000000000000000000000000000",
                                "excess_sig": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                                "features": "Plain",
                                "fee": "7000000",
                                "lock_height": "0"
                            }
                            ],
                            "outputs": [
                            {
                                "commit": "084ee97defa8c37124d4c69baa753e2532535faa81f79ea5e0489db25297d5beb8",
                                "features": "Plain",
                                "proof": "007df7dddd1efca757b2070740cc604628390eb59e151f96ff2eaa5361f5435fd1aa6ea3febc97fcfe1b3248d040c82de36180392976ba2d1147c2fb021c87ad044f1f9763934d9d3f4431417762eed03c53ce17aedb7824565c1f48fccec9c4abc0d28bd32b02ce9bee40bf6a60cf7c9c203cc24e4b779f901e12c987573698cf7f04e3aace26e71262138605424800adf3295d09f7f45dddf1855c785e98d45eae3cd111d18552e733895458df15e71a13838d789a4cb369f4ddb8aa9c503b080fd88a147245df0522d4136d36a183bd941e6cf94dffc78438b12194d4df7114d1e27a7a2f014920a321223ecbebb2b9642a22f8ed4e74883125f3e757b2f118853ffab1b68f15c1a2d021e583ff3fd1ea28720a81325b3cc2327ba9fb2fd9b2644adb7f3c7b2e319b2536a34f67e6f09346f24da6bcae1b241f8590493476dfe35b183e54f105eb219b601e0e53965409701dc1fd9562c42ad977505ea7bf264f01770569a4a358a70fb0b2c65969fac3b23954f0ca0adace0703243f1dab626509a8656e7a981709c3ac1d51694bafa55aad45c101937cbf3e45d6708c07be71419769a10a4f64f2b7d53a54eac73cdbd3279f91c5f8991a4b17621c36195a9391364fa221e8a8dee21ebc3a6eb9cd2940a3676e7ef3cdd46319bdc11f748785e49ff41bec2c3243255d83c6895bc0c893e6a772d7440a68321246b177709d3bd82d0dc2f5bca40c878e859b6f82319a386e0b7fcbc8010a25178b08418389ba7c6a77f99ac7f4ae5c686ab6574fcd0116f8573bccda3edfdff36c9c92ce2fb8bfb0ce2fe5c6b2498c6eb16fc2d40de9ddcba199a7e93d648abf39d6b248e196de7127e6b812e3080497f2a82afa69a471ab511e753e5b17a1c39c6728a065898af6674608d92a625e96e2f0258fe2eb06a27d0586d889d61f97faaa3facf58cda"
                            },
                            {
                                "commit": "0812276cc788e6870612296d926cba9f0e7b9810670710b5a6e6f1ba006d395774",
                                "features": "Plain",
                                "proof": "dcff6175390c602bfa92c2ffd1a9b2d84dcc9ea941f6f317bdd0f875244ef23e696fd17c71df79760ce5ce1a96aab1d15dd057358dc835e972febeb86d50ccec0dad7cfe0246d742eb753cf7b88c045d15bc7123f8cf7155647ccf663fca92a83c9a65d0ed756ea7ebffd2cac90c380a102ed9caaa355d175ed0bf58d3ac2f5e909d6c447dfc6b605e04925c2b17c33ebd1908c965a5541ea5d2ed45a0958e6402f89d7a56df1992e036d836e74017e73ccad5cb3a82b8e139e309792a31b15f3ffd72ed033253428c156c2b9799458a25c1da65b719780a22de7fe7f437ae2fccd22cf7ea357ab5aa66a5ef7d71fb0dc64aa0b5761f68278062bb39bb296c787e4cabc5e2a2933a416ce1c9a9696160386449c437e9120f7bb26e5b0e74d1f2e7d5bcd7aafb2a92b87d1548f1f911fb06af7bd6cc13cee29f7c9cb79021aed18186272af0e9d189ec107c81a8a3aeb4782b0d950e4881aa51b776bb6844b25bce97035b48a9bdb2aea3608687bcdd479d4fa998b5a839ff88558e4a29dff0ed13b55900abb5d439b70793d902ae9ad34587b18c919f6b875c91d14deeb1c373f5e76570d59a6549758f655f1128a54f162dfe8868e1587028e26ad91e528c5ae7ee9335fa58fb59022b5de29d80f0764a9917390d46db899acc6a5b416e25ecc9dccb7153646addcc81cadb5f0078febc7e05d7735aba494f39ef05697bbcc9b47b2ccc79595d75fc13c80678b5e237edce58d731f34c05b1ddcaa649acf2d865bbbc3ceda10508bcdd29d0496744644bf1c3516f6687dfeef5649c7dff90627d642739a59d91a8d1d0c4dc55d74a949e1074427664b467992c9e0f7d3af9d6ea79513e8946ddc0d356bac49878e64e6a95b0a30214214faf2ce317fa622ff3266b32a816e10a18e6d789a5da1f23e67b4f970a68a7bcd9e18825ee274b0483896a40"
                            }
                            ]
                        },
                        "offset": "d202964900000000d302964900000000d402964900000000d502964900000000"
                    },
                    "version_info": {
                        "orig_version": 2,
                        "version": 2,
                        "block_header_version": 2
                    }
                }
            }
        }
    */
    pServer->AddMethod("receive_tx", std::shared_ptr<RPCMethod>((RPCMethod*)new ReceiveTxHandler(walletManager, token, pTorProcess)));

    /*
        Request:
        {
            "jsonrpc": "2.0",
            "method": "check_version",
            "id": 1,
            "params": [
                {
                    "ver": "4:2",
                    "id": "0436430c-2b02-624c-2032-570501212b00",
                    "sta": "I2",
                    "off": "383bc9df0dd332629520a0a72f8dd7f0e97d579dccb4dbdc8592aa3d424c846c",
                    "fee": "23500000",
                    "sigs": [
                        {
                            "xs": "02e3c128e436510500616fef3f9a22b15ca015f407c8c5cf96c9059163c873828f",
                            "nonce": "031b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d5dd078f",
                            "part": "8f07ddd5e9f5179cff19486034181ed76505baaad53e5d994064127b56c5841be7bf31d80494f5e4a3d656649b1610c61a268f9cafcfc604b5d9f25efb2aa3c5"
                        }
                    ],
                    "coms": [
                        {
                            "f": 1,
                            "c": "087df32304c5d4ae8b2af0bc31e700019d722910ef87dd4eec3197b80b207e3045"
                        },
                        {
                            "f": 1,
                            "c": "08e1da9e6dc4d6e808a718b2f110a991dd775d65ce5ae408a4e1f002a4961aa9e7"
                        },
                        {
                            "c": "09ede20409d5ae0d1c0d3f3d2c68038a384cdd6b7cc5ca2aab670f570adc2dffc3",
                            "p": "6d86fe00220f8c6ac2ad4e338d80063dba5423af525bd273ecfac8ef6b509192732a8cd0c53d3313e663ac5ccece3d589fd2634e29f96e82b99ca6f8b953645a005d1bc73493f8c41f84fb8e327d4cbe6711dba194a60db30700df94a41e1fda7afe0619169389f8d8ee12bddf736c4bc86cd5b1809a5a27f195209147dc38d0de6f6710ce9350f3b8e7e6820bfe5182e6e58f0b41b82b6ec6bb01ffe1d8b3c2368ebf1e31dfdb9e00f0bc68d9119a38d19c038c29c7b37e31246e7bba56019bc88881d7d695d32557fc0e93635b5f24deffefc787787144e5de7e86281e79934e7e20d9408c34317c778e6b218ee26d0a5e56b8b84a883e3ddf8603826010234531281486454f8c2cf3fee074f242f9fc1da3c6636b86fb6f941eb8b633d6e3b3f87dfe5ae261a40190bd4636f433bcdd5e3400255594e282c5396db8999d95be08a35be9a8f70fdb7cf5353b90584523daee6e27e208b2ca0e5758b8a24b974dca00bab162505a2aa4bcefd8320f111240b62f861261f0ce9b35979f9f92da7dd6989fe1f41ec46049fd514d9142ce23755f52ec7e64df2af33579e9b8356171b91bc96b875511bef6062dd59ef3fe2ddcc152147554405b12c7c5231513405eb062aa8fa093e3414a144c544d551c4f1f9bf5d5d2ff5b50a3f296c800907704bed8d8ee948c0855eff65ad44413af641cdc68a06a7c855be7ed7dd64d5f623bbc9645763d48774ba2258240a83f8f89ef84d21c65bcb75895ebca08b0090b40aafb7ddef039fcaf4bad2dbbac72336c4412c600e854d368ed775597c15d2e66775ab47024ce7e62fd31bf90b183149990c10b5b678501dbac1af8b2897b67d085d87cab7af4036cba3bdcfdcc7548d7710511045813c6818d859e192e03adc0d6a6b30c4cbac20a0d6f8719c7a9c3ad46d62eec464c4c44b58fca463fea3ce1fc51"
                        }
                    ]
                }
            ]
        }

        Reply:
        {
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "Ok": {
                    "coms": [
                        {
                            "c": "087df32304c5d4ae8b2af0bc31e700019d722910ef87dd4eec3197b80b207e3045",
                            "f": 1
                        },
                        {
                            "c": "08e1da9e6dc4d6e808a718b2f110a991dd775d65ce5ae408a4e1f002a4961aa9e7",
                            "f": 1
                        },
                        {
                            "c": "099b48cfb1f80a2347dc89818449e68e76a3c6817a532a8e9ef2b4a5ccf4363850",
                            "p": "29701ceae262cac77b79b868c883a292e61e6de8192b868edcd1300b0973d91396b156ace6bd673402a303de10ddd8a5e6b7f17ba6557a574a672bd04cc273ab04ed8e2ca80bac483345c0ec843f521814ce1301ec9adc38956a12b4d948acce71295a4f52bcdeb8a1c9f2d6b2da5d731262a5e9c0276ef904df9ef8d48001420cd59f75a2f1ae5c7a1c7c6b9f140e7613e52ef9e249f29f9340b7efb80699e460164324616f98fd4cde3db52497c919e95222fffeacb7e65deca7e368a80ce713c19de7da5369726228ee336f5bd494538c12ccbffeb1b9bfd5fc8906d1c64245b516f103fa96d9c56975837652c1e0fa5803d7ccf1147d8f927e36da717f7ad79471dbe192f5f50f87a79fc3fe030dba569b634b92d2cf307993cce545633af263897cd7e6ebf4dcafb176d07358bdc38d03e45a49dfa9c8c6517cd68d167ffbf6c3b4de0e2dd21909cbad4c467b84e5700be473a39ac59c669d7c155c4bcab9b8026eea3431c779cd277e4922d2b9742e1f6678cbe869ec3b5b7ef4132ddb6cdd06cf27dbeb28be72b949fa897610e48e3a0d789fd2eea75abc97b3dc7e00e5c8b3d24e40c6f24112adb72352b89a2bef0599345338e9e76202a3c46efa6370952b2aca41aadbae0ea32531acafcdab6dd066d769ebf50cf4f3c0a59d2d5fa79600a207b9417c623f76ad05e8cccfcd4038f9448bc40f127ca7c0d372e46074e334fe49f5a956ec0056f4da601e6af80eb1a6c4951054869e665b296d8c14f344ca2dc5fdd5df4a3652536365a1615ad9b422165c77bf8fe65a835c8e0c41e070014eb66ef8c525204e990b3a3d663c1e42221b496895c37a2f0c1bf05e91235409c3fe3d89a9a79d6c78609ab18a463311911f71fa37bb73b15fcd38143d1404fd2ce81004dc7ff89cf1115dcc0c35ce1c1bf9941586fb959770f2618ccb7118a7"
                        },
                        {
                            "c": "09ede20409d5ae0d1c0d3f3d2c68038a384cdd6b7cc5ca2aab670f570adc2dffc3",
                            "p": "6d86fe00220f8c6ac2ad4e338d80063dba5423af525bd273ecfac8ef6b509192732a8cd0c53d3313e663ac5ccece3d589fd2634e29f96e82b99ca6f8b953645a005d1bc73493f8c41f84fb8e327d4cbe6711dba194a60db30700df94a41e1fda7afe0619169389f8d8ee12bddf736c4bc86cd5b1809a5a27f195209147dc38d0de6f6710ce9350f3b8e7e6820bfe5182e6e58f0b41b82b6ec6bb01ffe1d8b3c2368ebf1e31dfdb9e00f0bc68d9119a38d19c038c29c7b37e31246e7bba56019bc88881d7d695d32557fc0e93635b5f24deffefc787787144e5de7e86281e79934e7e20d9408c34317c778e6b218ee26d0a5e56b8b84a883e3ddf8603826010234531281486454f8c2cf3fee074f242f9fc1da3c6636b86fb6f941eb8b633d6e3b3f87dfe5ae261a40190bd4636f433bcdd5e3400255594e282c5396db8999d95be08a35be9a8f70fdb7cf5353b90584523daee6e27e208b2ca0e5758b8a24b974dca00bab162505a2aa4bcefd8320f111240b62f861261f0ce9b35979f9f92da7dd6989fe1f41ec46049fd514d9142ce23755f52ec7e64df2af33579e9b8356171b91bc96b875511bef6062dd59ef3fe2ddcc152147554405b12c7c5231513405eb062aa8fa093e3414a144c544d551c4f1f9bf5d5d2ff5b50a3f296c800907704bed8d8ee948c0855eff65ad44413af641cdc68a06a7c855be7ed7dd64d5f623bbc9645763d48774ba2258240a83f8f89ef84d21c65bcb75895ebca08b0090b40aafb7ddef039fcaf4bad2dbbac72336c4412c600e854d368ed775597c15d2e66775ab47024ce7e62fd31bf90b183149990c10b5b678501dbac1af8b2897b67d085d87cab7af4036cba3bdcfdcc7548d7710511045813c6818d859e192e03adc0d6a6b30c4cbac20a0d6f8719c7a9c3ad46d62eec464c4c44b58fca463fea3ce1fc51"
                        }
                    ],
                    "fee": "23500000",
                    "id": "0436430c-2b02-624c-2032-570501212b00",
                    "off": "a5a632f26f27a9b71e98c1c8b8098bb41204ffcfd206d995f9c16d10764ad95a",
                    "sigs": [
                        {
                            "nonce": "031b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d5dd078f",
                            "part": "8f07ddd5e9f5179cff19486034181ed76505baaad53e5d994064127b56c5841be7bf31d80494f5e4a3d656649b1610c61a268f9cafcfc604b5d9f25efb2aa3c5",
                            "xs": "02e3c128e436510500616fef3f9a22b15ca015f407c8c5cf96c9059163c873828f"
                        },
                        {
                            "nonce": "031b84c5567b126440995d3ed5aaba0565d71e1834604819ff9c17f5e9d5dd078f",
                            "part": "8f07ddd5e9f5179cff19486034181ed76505baaad53e5d994064127b56c5841b04e1e15ceb1b5dbab8baf7750d7bd4aad6cfe97b83e4dc080dae328eb75881fd",
                            "xs": "02e89cce4499ac1e9bb498dab9e3fab93cc40cd3d26c04a0292e00f4bf272499ec"
                        }
                    ],
                    "sta": "I3",
                    "ver": "4:2"
                }
            }
        }
    */
    pServer->AddMethod("finalize_tx", std::shared_ptr<RPCMethod>((RPCMethod*)new FinalizeTxHandler(walletManager, token, pTorProcess)));

    pServer->GetServer()->AddListener("/status", StatusListener, nullptr);
    WALLET_DEBUG_F("Foreign RPC API running at port: {}", pServer->GetPortNumber());
    
    std::optional<TorAddress> addressOpt = AddTorListener(keyChain, pTorProcess, pServer->GetPortNumber(), currentAddressIndex);

    return std::make_unique<ForeignServer>(pTorProcess, pServer, addressOpt);
}

std::optional<TorAddress> ForeignServer::AddTorListener(
    const KeyChain& keyChain,
    const TorProcess::Ptr& pTorProcess,
    const uint16_t portNumber,
    int addressIndex)
{
    if (!pTorProcess) {
        return std::nullopt;
    }

    try
    {
        KeyChainPath path = KeyChainPath::FromString("m/0/1").GetChild(addressIndex);
        ed25519_keypair_t torKey = keyChain.DeriveED25519Key(path);
        auto tor_address = pTorProcess->AddListener(torKey.secret_key, portNumber);
        WALLET_DEBUG_F("Adding Tor Listener at: {}", path.Format());
        return std::make_optional(std::move(tor_address));
    }
    catch (std::exception& e)
    {
        WALLET_ERROR_F("Exception thrown: {}", e.what());
    }

    return std::nullopt;
}