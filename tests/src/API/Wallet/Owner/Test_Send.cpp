#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>
#include <API/Wallet/Owner/Models/SendResponse.h>

#include <TestServer.h>
#include <TestChain.h>
#include <TestWalletServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: send - Slatepack")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);
    auto pWalletManager = pTestWalletServer->GetWalletManager();

    auto pSenderWallet = pTestWalletServer->CreateUser("Alice", "P@ssw0rd123!");
    auto pReceiverWallet = pTestWalletServer->CreateUser("Bob", "P@ssw0rd123!", false);

    TestChain chain(pTestServer);
    chain.MineChain(pSenderWallet, 30);

    // Send
    SendCriteria criteria(
        pSenderWallet->GetToken(),
        1'000'000'000,
        1'000'000,
        (uint8_t)1,
        SelectionStrategyDTO(ESelectionStrategy::SMALLEST, {}),
        std::make_optional<std::string>(pReceiverWallet->GetSlatepackAddress().ToString()),
        std::nullopt
    );

    RPC::Request request = RPC::Request::BuildRequest("send", criteria.ToJSON());

    auto rpc_response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);
    REQUIRE(rpc_response.GetResult().has_value());

    SendResponse response = SendResponse::FromJSON(rpc_response.GetResult().value());
    REQUIRE(response.GetStatus() == SendResponse::EStatus::SENT);

    // Unpack slatepack
    auto wallet = pWalletManager->GetWallet(pReceiverWallet->GetToken());
    SlatepackMessage decrypted = wallet.Read()->DecryptSlatepack(response.GetArmoredSlate());

    ByteBuffer deserializer(decrypted.m_payload);
    REQUIRE(Slate::Deserialize(deserializer) == response.GetSlate());

    // TODO: Check amount, fee?, etc
}

//TEST_CASE("API: send - TOR")
//{
//    TestServer::Ptr pTestServer = TestServer::Create();
//    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);
//    TestChain chain(pTestServer);
//
//    auto pSender = pTestWalletServer->CreateUser("Sender", "P@ssw0rd123!");
//    REQUIRE(pSender->GetTorAddress().has_value());
//
//    auto pReceiver = pTestWalletServer->CreateUser("Receiver", "password");
//    REQUIRE(pReceiver->GetTorAddress().has_value());
//    std::cout << pReceiver->GetTorAddress().value().ToString() << std::endl;
//
//    chain.MineChain(pSender, 30);
//
//    Json::Value paramsJson;
//    paramsJson["session_token"] = pSender->GetToken().ToBase64();
//    paramsJson["amount"] = 1'000'000'000;
//    paramsJson["fee_base"] = 1'000'000;
//    Json::Value selectionStrategyJson;
//    selectionStrategyJson["strategy"] = SelectionStrategy::ToString(ESelectionStrategy::SMALLEST);
//    paramsJson["selection_strategy"] = selectionStrategyJson;
//    paramsJson["address"] = pReceiver->GetTorAddress().value().ToString();
//
//    std::this_thread::sleep_for(std::chrono::seconds(15));
//
//    RPC::Request request = RPC::Request::BuildRequest("send", paramsJson);
//
//    std::unique_ptr<RPC::Response> pResponse = nullptr;
//    std::thread thread([&request, pTestWalletServer, &pResponse]() {
//        try
//        {
//            auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);
//            pResponse = std::make_unique<RPC::Response>(std::move(response));
//        }
//        catch (std::exception& e)
//        {
//            std::cout << e.what() << std::endl;
//        }
//    });
//    //auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);
//    thread.join();
//
//    LoggerAPI::Flush();
//
//    REQUIRE(pResponse != nullptr);
//
//    REQUIRE(!pSender->GetToken().ToBase64().empty());
//    REQUIRE(!pReceiver->GetToken().ToBase64().empty());
//
//    auto resultOpt = pResponse->GetResult();
//    REQUIRE(resultOpt.has_value());
//
//    const Json::Value& json = resultOpt.value();
//    REQUIRE(json["status"] == "FINALIZED");
//    const Slate slate = Slate::FromJSON(json["slate"]);
//
//    std::vector<WalletTxDTO> txs = pReceiver->GetTransactions();
//    REQUIRE(txs.size() == 1);
//
//    REQUIRE(txs[0].GetKernels().size() == 1);
//    //const Commitment& kernelCommitment = txs[0].GetKernels().front();
//    //REQUIRE(slate.GetTransaction().GetKernels().front().GetCommitment() == kernelCommitment);
//
//    REQUIRE(txs[0].GetOutputs().size() == 1);
//
//    auto pTransaction = pTestServer->GetTxPool()->GetTransactionToStem(
//        pTestServer->GetDatabase()->GetBlockDB()->Read().GetShared(),
//        pTestServer->GetTxHashSetManager()->Read()->GetTxHashSet()
//    );
//
//    REQUIRE(pTransaction != nullptr);
//    REQUIRE(pTransaction->GetInputs().size() == 1);
//    REQUIRE(pTransaction->GetOutputs().size() == 2);
//}