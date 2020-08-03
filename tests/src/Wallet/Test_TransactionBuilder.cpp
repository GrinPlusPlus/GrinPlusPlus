#include <catch.hpp>

#include <Core/Models/Transaction.h>
#include <Core/Validation/TransactionValidator.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/SlateBuilder/TransactionBuilder.h>
#include <Crypto/Crypto.h>

TEST_CASE("Debugging")
{
    std::string kernel_json_str = R"({
                "excess": "08b4aa87dc21a03075dbfca8519c53d95efe110c4f06b349a02e935744e7fb0da7",
                "excess_sig": "47244b864c366be0a2b479a46e5d4387b0ab0e0d9b5dc1d703b9333c7d8dc9a1cf66a6d4242aeb85018a35bad5402d929c746cfbec40b8c2db00d59c9514b606",
                "features": {
                    "Plain": {
                        "fee": 7000000
                    }
                }
            })";
    Json::Value kernel_json;
    REQUIRE(JsonUtil::Parse(kernel_json_str, kernel_json));
    TransactionKernel kernel = TransactionKernel::FromJSON(kernel_json);

    PublicKey excess = PublicKey::FromHex("03b79714dba5bf9e03518c37afa9178307cd25c698eb49618285ec734cf1bbcb6d");
    //PublicKey nonce = PublicKey::FromHex("03ce37d20f304e6d1e8f0807ed6fd50cb1c3f939838141925675504400c4557a92");
    CompactSignature signature = CompactSignature(CBigInteger<64>::FromHex("927a55c400445075569241818339f9c3b10cd56fed07088f1e6d4e300fd237ce419972f9af77b7dcf146022347f4a22e0356abb242529d98739124e3dabc386a"));

    PublicKey total_excess = PublicKey::FromHex("03b4aa87dc21a03075dbfca8519c53d95efe110c4f06b349a02e935744e7fb0da7");
    PublicKey total_nonce = PublicKey::FromHex("03a1c98d7d3c33b903d7c15d9b0d0eabb087435d6ea479b4a2e06b364c864b2447");

    const bool partial_valid = Crypto::VerifyPartialSignature(signature, excess, total_excess, total_nonce, kernel.GetSignatureMessage());
    REQUIRE(partial_valid);
}

TEST_CASE("Debugging2")
{

    std::string transaction_json_str = R"({
        "body": {
            "inputs": [{
                    "commit": "09c71522e76e98dd07ece241a30a07d71135749550d316bcce0d7b31340383c7fe",
                    "features": "Plain"
                }, {
                    "commit": "09a7c4126740fe3f2504a8d0036e0f0f1afeda542c39621783ca7c7ed7ea2eecfe",
                    "features": "Plain"
                }
            ],
            "kernels": [{
                    "excess": "0985818724adbd5efd9d543b71b2d13f24bd71171045252ca0ef6cc35dd91a33cd",
                    "excess_sig": "7ea0213ba736828ff29e4ea2c7ccc61634cf52235ad98df5622dde478ce8e2b623590958b7b21d256f6923b5f2b3d348c85401cd15f773901d36a10bd431f5ca",
                    "features": {
                        "Plain": {
                            "fee": 7000000
                        }
                    }
                }
            ],
            "outputs": [{
                    "commit": "09dc71bb231cff82d2c2dca30f7e073987f5c03f2cd8cc88501869d0fc258c20ef",
                    "features": "Plain",
                    "proof": "66ed6c54cbc07e376861191d2a68c7f4cd00f6ece831f62f1ae5319c9fc282bfc7406a96c2d5036034e96540c6e0c7964f16e1a4855836fe982b800244ba29430842d118a8f0659c9579897eca93e9787869ea791531e2a5b6306158b937ce35cca0437c6b80d60d01236d6814a71902a259d36e981da0141f2452a987297f7bb5c1a6c5c12632122a5dc707880ff4edeb5046afaee1006cfae26015d5a556be2f556099bc3c59ee427193e4689d9fb15363ac5ab98c65aedc00ba9b2cb745c821ab66c89dbe0583917b08f842e01b1b59983732227aff11818506134f0ed2f9e24ea62c7862d1cfe819bed9e206c9687d78f20ed63c79c3366974f750eca4a3f6508b81f03548c6d0286c7dabc58c9f530b443f68df0a46efb2e0d0b15bcaec2ec5c5a1facd0e8875d47da73db40ec2938f324840882f34621826b8c938f4acb5adc96bdaa3f8351ecd51debdd7fa7d535eb241faa282de7e3409d722c9822dea3603ae49c2b265a8bd55d236aedbeb234abd34a1690ec4938b251a4b344c60f2528b2c21ed3acecc01f201a73436c4f0a450e6bd6eadb8b173e8e7569e2f7ab9b7a251034249c59b156950807c350c48b7709812fd2382552ac0052ff7511ec5b372c8ca54581c9f532d98fb2a2ae6e4d2bc1047b331b03f254d1a84ef7807f1151729ce93fbb6076caa04acf9cf689aea85ad066795ef515dab71610c886e6665960a1607e2ad771462e7c46af60e8f7b0ef6d8126b636ebaf3f9349d8c45d8f9657ed9ebf58531555b5176d5626c01440a264fb17c7a74bf14a5d7ea1c1dad5297eb7f09eee126456fb35b254d524e1eed455bcd05f94ac2ee6f5bc9fb6d819144e65338d381fa3c8229c1b7c7309b8dd429188e8268027f30553f0f94511679cdb2ebc632d28ae019ae8358d10158ad323c1fbaee6feaf03cc925c9057a8ff909"
                }, {
                    "commit": "09f75187bfdd9073dfc487f59d474bd757937c70c65d59d9c9e6a752523252d7be",
                    "features": "Plain",
                    "proof": "ca93e6e0912679bebbae0ebe416b41e3a70064c629fc806322e48bea2ba6609d42cf1150807692455c139872f7c71e795aea11b30444ed5fd6465a5e0ba60a6e0237a00a61e5a8fa9e885dd4e6d2d6712b0e34267a2a3ba5554e12b1efd7df6a3af1201561f627fc4d554e2644ebf0e1af26d8e2484d82f3dfd96d29e90c93b018e46e63371ac1971022fc65854aaf3bd02776fa4b9327e78ee376719f6b1970c594789094097c20f36572de1d145e366f2db5426153bda24fd35842b1f0674b75caf8746260f393354b8a3354ddf5c64dcaed43470c9fbc6b6b97376485309ddb155ddefbe9b32e54c6d573e1473588d2609bbf169a2f3b6ab5fa151f2e10e9a218a33477bf67a30572f43d33d349d87a834a8029fe622d0723f25c521eec3f251898ca02e4734df228945db5f0ea9ed04afb58a31f9e422362ded3f07f27078cbb59fdf23f53ca33fc830fa37d4e106ae44cc842297937be16badb8e188b4a555b004a53be0d2a60fcfa9e403033c82135aa9a900b603f0669be9a172419ee3d67bd90b75f388c9443923f391f6b0512e49b880791c87ffb0f96096949c63d0258b33189fd91bd94ed5cc2f819456d2f9285a03bac6df902a4498e12ec8c4c7ef572ceedb7fe94cda6f7ef26ee88f5cd217ecb2b270735e937942dc93b821d3abd5081c1eeba2f4e3e401f7e409d13f8e0b9cdaaf75d3d0ffe9353a0c0107a426587ffcc8f622399afae1f10de247a767ce7618509076aee1e22ec191687d9ecac2034b87e4870b0b71611de47f643302d2a11a465f7656f9aa8ef1133135e4b7973ac96a379101a883d1d3069979163fa23a94b3023926d166e5527257740dafa67934783f6a90e5d27c6e76332a53b018c97bbc66a0568d5eb429d7bc2ab4c406d699a0534dae8ac313b25f23425cde5cf8d8dc9c4c42f37aac1b918cad78678fd"
                }
            ]
        },
        "offset": "f2c1f67e42884393ecbb9728a7cff6142b8de02dbb57f78bb2e86d42be19c255"
    })";

    Json::Value transaction_json;
    REQUIRE(JsonUtil::Parse(transaction_json_str, transaction_json));
    Transaction transaction = Transaction::FromJSON(transaction_json);


    PublicKey excess = PublicKey::FromHex("03a13f9050ccae3ae532865d7050d9e207fcb993f42bedf003fdcf2fbea8e455c4");
    //PublicKey nonce = PublicKey::FromHex("03ce37d20f304e6d1e8f0807ed6fd50cb1c3f939838141925675504400c4557a92");
    CompactSignature signature = CompactSignature(CBigInteger<64>::FromHex("89a262dafb13e64b632a024abf8a864b34eea7e254c7a93fe0fa601c852411ea83dc2fa2691bf705d0cd40ff11205b3609d8cb3a2960ea0645ee591d3a72574f"));

    PublicKey total_excess = PublicKey::FromHex("0285818724adbd5efd9d543b71b2d13f24bd71171045252ca0ef6cc35dd91a33cd");
    PublicKey total_nonce = PublicKey::FromHex("03b6e2e88c47de2d62f58dd95a2352cf3416c6ccc7a24e9ef28f8236a73b21a07e");

    Commitment total_excess_commitment = Crypto::ToCommitment(total_excess);
    REQUIRE(total_excess_commitment == Commitment::FromHex("0985818724adbd5efd9d543b71b2d13f24bd71171045252ca0ef6cc35dd91a33cd"));

    REQUIRE(transaction.GetKernels().front().GetSignatureMessage().ToHex() == "bd5562c4e3aa5de611a7dd79b7aa0ef7ae4c9cfe58b1cc986535a5018d8799dd");
    const bool partial_valid = Crypto::VerifyPartialSignature(signature, excess, total_excess, total_nonce, transaction.GetKernels().front().GetSignatureMessage());
    REQUIRE(partial_valid);
}

//TEST_CASE("TransactionBuilder")
//{
//    std::string unsorted_transaction_str = R"({
//    "body": {
//        "inputs": [{
//                "commit": "08411ad29f83e3a4f2c2add5626f26cd4a7794de4185cba4ec407215bacc009977",
//                "features": "Plain"
//            }, {
//                "commit": "09eed778ee4ba0572f4aaf0021e5edae4f95641bcef8170ce6e81c77a2d06d8a86",
//                "features": "Plain"
//            }, {
//                "commit": "0800416cdade02dcd28e32a4e9fc7319c995cd1cfbe3a998a0b2d606d79a29ecb8",
//                "features": "Plain"
//            }, {
//                "commit": "09c1ff2ffaf39563740e250202b4a0c387a1962923d671645aed317667152204ec",
//                "features": "Plain"
//            }, {
//                "commit": "08286bfbe7793d39f7f94013ca21018b23d78074e0db80cffe12e82fa0368f81b5",
//                "features": "Plain"
//            }, {
//                "commit": "0864b7e84e8a0107215b29435fda6a89bf038d6238f3de86fe1c68e86aa47be06d",
//                "features": "Plain"
//            }, {
//                "commit": "08ae1494619e560b37f4f5c615dfd6243885efc104824e011942906831b8ad9696",
//                "features": "Plain"
//            }, {
//                "commit": "09e137bf12651d14faff882f8d421644023643bf9a060f6f51db2865d7b5da6188",
//                "features": "Plain"
//            }, {
//                "commit": "09f45fdc94d9c71a9797bbf44055f574924c53819c5b8a4b37ccf1b24327fcdf5c",
//                "features": "Plain"
//            }, {
//                "commit": "08906fbc59cc43b24acdaa957ccd3d91dd3f8b79a90610181810fbf55d38b03116",
//                "features": "Plain"
//            }, {
//                "commit": "09c8173db0ccced73e5a7a82777fdaecacbf6d0792d23a05454e2d10c7a39e98ca",
//                "features": "Plain"
//            }, {
//                "commit": "098627dd0600e07424f39fc06ccfe684cde9602fc0d2a0d87b597087d5eef4fc26",
//                "features": "Plain"
//            }, {
//                "commit": "085d5ed5816f38d3cfd1251443f87457033c8766ae2563f1fb80ecb8b562c92b15",
//                "features": "Plain"
//            }, {
//                "commit": "0936ef0719122dc4a256852d2fcbdb4e93a09d48d70a1bcfb1b66be4f38d4c2c65",
//                "features": "Plain"
//            }, {
//                "commit": "0901225158de0cb61195d26ac4588321cc786afdd831073ef3db1e42a980c9945c",
//                "features": "Plain"
//            }, {
//                "commit": "09a2948afbff97ded3ae4602dda79d02f78357eb7ddbe2b9cb21a0d32c028e902a",
//                "features": "Plain"
//            }, {
//                "commit": "0900b393409db586cb4416ba785564884130dab5e4ebd7f4285f2167dda5885e28",
//                "features": "Plain"
//            }, {
//                "commit": "09fbfe7cc6b87523c5aa5e63ad56427448bbdd3134fbc4f78684a5f2875ca51274",
//                "features": "Plain"
//            }, {
//                "commit": "09a26ab39c7e4278aefcb707b2085ada80adbb097adad2263f13763029c21876ef",
//                "features": "Plain"
//            }, {
//                "commit": "09c50ce88e79a41a78a9f40feceba47fd2723dcf1253352ca241d1fbde614a195a",
//                "features": "Plain"
//            }, {
//                "commit": "08c9031b6a2a5bd7c6e23e752de4c9a2bf1fb8da44c02c01e9f2eb3c51c2f9deed",
//                "features": "Plain"
//            }, {
//                "commit": "094074a8aa56cae50648cf45a7342068ae2d287f30ffc70f6cdcc099662ee95101",
//                "features": "Plain"
//            }, {
//                "commit": "090e06e20a1f0138a2428d490505593808d9d5001ef7b6fdadfc9968d9a82b1cf8",
//                "features": "Plain"
//            }, {
//                "commit": "092aedcaa826d5ddc3901091f5b2895b58c85355f560f0ccf338e4fcfbc62cca11",
//                "features": "Plain"
//            }, {
//                "commit": "09c0313ff3166057b4f5eabe240c0c1a24256334b1df4612faf21406d92be5ea26",
//                "features": "Plain"
//            }, {
//                "commit": "08af4603fd6db18fa7da96c494bae98fc21116dff088d04da4b1de81d88f3a875e",
//                "features": "Plain"
//            }, {
//                "commit": "08fbe2267c32a4005efe2d05ff729eb48cad02d94ebc49c7db0c91a63d6f859ce6",
//                "features": "Plain"
//            }, {
//                "commit": "08109abbb4eb5956f7120b4280d3ed9428e2527857c058208d52935f7f802e7cad",
//                "features": "Plain"
//            }, {
//                "commit": "09c6e9e8956c45d6d8f15beafa4d24b96427cba04f8682ff1f5ac4e7f8fe9430cb",
//                "features": "Plain"
//            }, {
//                "commit": "09f0110c79bea58e9670620690ab2d9325c838b7dbca06dbda98dfba81a7718d12",
//                "features": "Plain"
//            }
//        ],
//        "kernels": [{
//                "excess": "09ee5f3fd3464c70ba5aac9422f721c29cd9a37ff66634cfa59c1cdf0bcaf6832d",
//                "excess_sig": "42d29962f1424b694d47698a559ddb081309af4d5523b987a980ab6cff1edac2c0e76adf62795b6715cad8f302af06aedc338b9696809c05b14e9833ced28d38",
//                "features": {
//                    "Plain": {
//                        "fee": "1000000"
//                    }
//                }
//            }
//        ],
//        "outputs": [{
//                "commit": "09afa0413af63938abd451b60896851cc61adb499e44be50e881142e4215d2483b",
//                "features": "Plain",
//                "proof": "8fff3cd7d6dbd1550317fb1eb91e64be959768f718b691036c92dc99bda1021ed3893eecf654d4c05b3b1c3206cbd8bb9ed887fa8331a43f6ec4d94b7c9752a8041df842cfb48c97905e8f4cac403caa56e4a2439ccf905ebb19e7825bd078e03412b2ddf65ff9c24c9f1e7b5c333d8d0cb6d11d5889606f7322bdfc046fde3783a4ce50c3769f4fde3e5314b90368a958680c04d12ae004c3bb7615e4cd4e3a50088dfb1892c6f8d3b7f96713262713c94dc4f55e0e8997ba002e3b58e40a29dbac045a047772b72bb28007ce752a7a16461ee112679e4cb4e5b867a50f52598c64ce22fbb407af88c820f5d9a73afb59f54e8c61e4150661316e51148c6a53648303957df5af340c7d290ede8bc094e2fc0a16bc916499b12ed388949d318a2b5545fc6640e7f451dabfcf39f65133fdae806dc32e27bea11df33ab93240b11f761e60434cc380562577ef87fae85e8f0bde74cb1f6148fe1808d714a2fa9cf8e203d12d571a0b77f5095daf8a03a1d6a4f246998268f2bb0a279ea9cee40f1ebec1dffd348ffed56e4dc9a102f55bd7ec26e1f15ef13f0bfa574629264545cf323ec71c825ba66ef88e9073ee6d2dad742854d31ac4d29ffac92ba72ea4835b51b74f1417b388e96b2f84a4f0d2575c7c799aa223ea399c79814a0ed80ab93ce36896a9e36dd9eac2a3124bbfd634768f437a11080a1050b8a2467790f4edb2383f000926bdd20b46dc36bdc90ba70b8c7111c17b80daa5ec945b4c7b782ecf5c88b5f8363781d026dee9abc42a89d2b4c6f72f8205edf777e655075fb5d8b60fc7ed5c5ef7a23827303aff162eb7d12760446f927446ecbfadb301260bb5ad7b3ee78075ef144620640940dccca8fadaa9af3b665ea26315d5ea6e09cc224c35fe1f75384252583619c74dae22f2bbd12018ce2616d24b1a6ce7cfd5250939cbc2"
//            }, {
//                "commit": "09abd5ec0941a8c6b8f3d84ce99548c694432e54ac56c9a5b7a7a6ae57764835b5",
//                "features": "Plain",
//                "proof": "b62bf13d476052033be87f527b625af14316b6bde29f0e0c0000a97b69dc2c08116ee06d808760d9cdb070642f0e17c0a5cc09a95b1de84f4afd9c5d8f1843080e026ce3822dfd0ce4f623dc44471fe614fb6e5f2ec64a7b8d66e479f7d9825a4580b00c79cf78b96104b636c5455614917762ed3f976bd08600c1f64f583e2538f176f5f2147b816c3b1414b0f4a46687f416554355c7b22bb8b2c3c049cc629ebad5db34b1169fa60b7e92e80de120a03c488b6fa2706d03b41e010695ffaa5636b68155aa1c71452887772d30d46e356e243a82d5d0a2885f5779516657d5c4e24205164bb0c39a03bf4874a132158579b5147e440e26fe800336958a82855c554c8207351a8c69d7449c95a9fcd09996c66dbfa9fe2c753b018f7425b80c74d4e8346d2b15ef81b6bab4ad97cd28f4176571bfa52b5c7f260b73345eec3966e5135fc368d9984aeab480dd5e7c398039edd5f8eb5c7f1b41f636a70fcf8faca1029067021fbb2f7968645f919708a752312030ea3a6275b4ab0889a1e9d2aa06f6a2a40b730d7430d6873df8b6169b697a7731036a4c5c0079511ca119d9cdb0930585204ca74261d0ec87be29030aa65424c15781f1052d59e549ee019b6c58dff0d3ad9951ddc47c99021c20003a8ddd915cc566a4df92612ecdd6b64becc80ab9ec22c8974e8aa67124d27155f0f696cbcb3c33309bd9881e0c2cee32f91b5790d9a349997cb6780073ae86ff3416cf1287473a9d0023c4dcdeb59d801f0559ec5d2b36e03f0dda17e38a1db8a94b0ba94ba18266ae1f7c047697af8c0d2bb245a937b28f288e9e722e8039add173097249d2fa0435de73bf6c99c5b21d470b770d5e5324a77eacebb879fa3fc7bce675e454fb0e9683d60ee2d26b0970ff8582d74ac2c0eaebfc3306c25a6e02b5dab9e83c34e5aaf5252a00ac56b4e35cad"
//            }
//        ]
//    },
//    "offset": "43a61f66692ce724bbaa85e4945c52595e04f31be0dbd7cd96be0697bbb679ab"
//})";
//    Json::Value json;
//    REQUIRE(JsonUtil::Parse(unsorted_transaction_str, json));
//    Transaction transaction = Transaction::FromJSON(json);
//
//    TransactionValidator().Validate(transaction);
//
//    CompactSignature sig1(CBigInteger<64>::FromHex("2e42b978bf6a6ed212c753543b69ffd2d15e61b6fc72649c08fd7fab644d6e359159eb55a77f7a28ebb7e6ccdc47466e3ffa2fd5227643cfbe29ec737f1369c4"));
//    CompactSignature sig2(CBigInteger<64>::FromHex("83616fc2512536509cda9415bd16c656de3c4d00bee0af2ca5eb0866228bcff9fac7a4324082864083845b14957331ce7683bfa823b3830c2244647c1140e077"));
//
//    PublicKey nonce1 = PublicKey::FromHex("03356e4d64ab7ffd089c6472fcb6615ed1d2ff693b5453c712d26e6abf78b9422e");
//    PublicKey nonce2 = PublicKey::FromHex("03f9cf8b226608eba52cafe0be004d3cde56c616bd1594da9c50362551c26f6183");
//    PublicKey sumNonces = Crypto::AddPublicKeys({ nonce1, nonce2 });
//    auto pSignature = Crypto::AggregateSignatures({ sig1, sig2 }, sumNonces);
//
//    Commitment commitment = Crypto::ToCommitment(PublicKey::FromHex("0244472ceded53beea1c5529d72eb805a20ea211d2b63ff77386ee0e80b5305e68"));
//    Hash message = TransactionKernel::GetSignatureMessage(EKernelFeatures::DEFAULT_KERNEL, 1000000, 0);
//
//    const bool valid = Crypto::VerifyKernelSignatures({ pSignature.get() }, { &commitment }, { &message });
//    REQUIRE(valid);
//}