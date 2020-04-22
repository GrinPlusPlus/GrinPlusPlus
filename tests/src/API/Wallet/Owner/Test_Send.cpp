#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>

#include <TestServer.h>
#include <TestChain.h>
#include <TestWalletServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: send - By File")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);
    TestChain chain(pTestServer);

    auto pWallet = pTestWalletServer->CreateUser("David", "P@ssw0rd123!");
    chain.MineChain(pWallet, 30);

    Json::Value paramsJson;
    paramsJson["session_token"] = pWallet->GetToken().ToBase64();
    paramsJson["amount"] = 1'000'000'000;
    paramsJson["fee_base"] = 1'000'000;
    Json::Value selectionStrategyJson;
    selectionStrategyJson["strategy"] = SelectionStrategy::ToString(ESelectionStrategy::SMALLEST);
    paramsJson["selection_strategy"] = selectionStrategyJson;

    fs::path file = pTestServer->GenerateTempDir() / "sent.tx";
    paramsJson["file"] = file.string(); // TODO: Test unicode

    RPC::Request request = RPC::Request::BuildRequest("send", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    const Json::Value& slateJson = resultOpt.value();

    const Slate slate = Slate::FromJSON(slateJson["slate"]);

    std::vector<uint8_t> bytes;
    REQUIRE(FileUtil::ReadFile(file, bytes));

    const Slate fileSlate = Slate::FromJSON(JsonUtil::Parse(bytes));

    REQUIRE(slate == fileSlate);
}

TEST_CASE("API: send - TOR")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);
    TestChain chain(pTestServer);

    auto pSender = pTestWalletServer->CreateUser("Sender", "P@ssw0rd123!");
    REQUIRE(pSender->GetTorAddress().has_value());

    auto pReceiver = pTestWalletServer->CreateUser("Receiver", "password");
    REQUIRE(pReceiver->GetTorAddress().has_value());
    std::cout << pReceiver->GetTorAddress().value().ToString() << std::endl;

    chain.MineChain(pSender, 30);

    Json::Value paramsJson;
    paramsJson["session_token"] = pSender->GetToken().ToBase64();
    paramsJson["amount"] = 1'000'000'000;
    paramsJson["fee_base"] = 1'000'000;
    Json::Value selectionStrategyJson;
    selectionStrategyJson["strategy"] = SelectionStrategy::ToString(ESelectionStrategy::SMALLEST);
    paramsJson["selection_strategy"] = selectionStrategyJson;
    paramsJson["address"] = pReceiver->GetTorAddress().value().ToString();

    std::this_thread::sleep_for(std::chrono::seconds(15));

    RPC::Request request = RPC::Request::BuildRequest("send", paramsJson);

    std::unique_ptr<RPC::Response> pResponse = nullptr;
    std::thread thread([&request, pTestWalletServer, &pResponse]() {
        try
        {
            auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);
            pResponse = std::make_unique<RPC::Response>(std::move(response));
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    });
    //auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);
    thread.join();

    LoggerAPI::Flush();

    REQUIRE(pResponse != nullptr);

    REQUIRE(!pSender->GetToken().ToBase64().empty());
    REQUIRE(!pReceiver->GetToken().ToBase64().empty());

    auto resultOpt = pResponse->GetResult();
    REQUIRE(resultOpt.has_value());

    const Json::Value& json = resultOpt.value();
    REQUIRE(json["status"] == "FINALIZED");
    const Slate slate = Slate::FromJSON(json["slate"]);

    std::vector<WalletTxDTO> txs = pReceiver->GetTransactions();
    REQUIRE(txs.size() == 1);

    REQUIRE(txs[0].GetKernels().size() == 1);
    const Commitment& kernelCommitment = txs[0].GetKernels().front();
    REQUIRE(slate.GetTransaction().GetKernels().front().GetCommitment() == kernelCommitment);

    REQUIRE(txs[0].GetOutputs().size() == 1);

    auto pTransaction = pTestServer->GetTxPool()->GetTransactionToStem(
        pTestServer->GetDatabase()->GetBlockDB()->Read().GetShared(),
        pTestServer->GetTxHashSetManager()->Read()->GetTxHashSet()
    );

    REQUIRE(pTransaction != nullptr);
    REQUIRE(pTransaction->GetInputs().size() == 1);
    REQUIRE(pTransaction->GetOutputs().size() == 2);
}