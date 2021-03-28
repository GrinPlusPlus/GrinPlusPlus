#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>
#include <Wallet/Models/DTOs/FeeEstimateDTO.h>

#include <TestServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: estimate_fee - SMALLEST")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    auto created_user = pTestServer->CreateUser("David", "P@ssw0rd123!");
    pTestServer->MineChain(created_user.wallet->GetKeychain(), 30);

    Json::Value paramsJson;
    paramsJson["session_token"] = created_user.wallet->GetToken().ToBase64();
    paramsJson["amount"] = Json::UInt64(150'000'000'000);
    paramsJson["fee_base"] = 1'000'000;
    Json::Value selectionStrategyJson;
    selectionStrategyJson["strategy"] = SelectionStrategy::ToString(ESelectionStrategy::SMALLEST);
    paramsJson["selection_strategy"] = selectionStrategyJson;
    paramsJson["change_outputs"] = 1;

    auto response = pTestServer->InvokeOwnerRPC("estimate_fee", paramsJson);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    const auto value = FeeEstimateDTO::FromJSON(resultOpt.value());
    REQUIRE(value.GetFee() == 48'000'000);

    // TODO: Finish assertions
}