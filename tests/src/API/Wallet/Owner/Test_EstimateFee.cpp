#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>
#include <Wallet/Models/DTOs/FeeEstimateDTO.h>

#include <TestServer.h>
#include <TestChain.h>
#include <TestWalletServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: estimate_fee - SMALLEST")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);
    TestChain chain(pTestServer);

    auto pWallet = pTestWalletServer->CreateUser("David", "P@ssw0rd123!");
    chain.MineChain(pWallet, 30);

    Json::Value paramsJson;
    paramsJson["session_token"] = pWallet->GetToken().ToBase64();
    paramsJson["amount"] = Json::UInt64(150'000'000'000);
    paramsJson["fee_base"] = 1'000'000;
    Json::Value selectionStrategyJson;
    selectionStrategyJson["strategy"] = SelectionStrategy::ToString(ESelectionStrategy::SMALLEST);
    paramsJson["selection_strategy"] = selectionStrategyJson;
    paramsJson["change_outputs"] = 1;

    auto response = pTestWalletServer->InvokeOwnerRPC("estimate_fee", paramsJson);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    const auto value = FeeEstimateDTO::FromJSON(*resultOpt);
    REQUIRE(value.GetFee() == 6'000'000);

    // TODO: Finish assertions
}