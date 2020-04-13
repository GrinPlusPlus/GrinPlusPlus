#include <catch.hpp>

#include <API/Wallet/Foreign/ForeignServer.h>
#include <Net/Clients/RPC/RPCClient.h>

#include <TestServer.h>
#include <Comparators/JsonComparator.h>

TEST_CASE("CheckVersionResponse")
{
    Json::Value okJson;
    okJson["foreign_api_version"] = (uint64_t)2;

    Json::Value slateVersionsJson;
    slateVersionsJson.append("V2");
    slateVersionsJson.append("V3");
    okJson["supported_slate_versions"] = slateVersionsJson;

    Json::Value json;
    json["Ok"] = okJson;

    // FromJSON
    {
        CheckVersionResponse response = CheckVersionResponse::FromJSON(json);

        REQUIRE(response.GetForeignApiVersion() == 2);
        REQUIRE(response.GetSupportedSlateVersions() == std::vector<uint64_t>{ 2, 3 });
    }

    // ToJSON
    {
        CheckVersionResponse response(2, { 2, 3 });
        REQUIRE(JsonComparator().Compare(json, response.ToJSON()));
    }
}

TEST_CASE("API: check_version")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    IWalletManagerPtr pWalletManager = WalletAPI::CreateWalletManager(
        *pTestServer->GetConfig(),
        pTestServer->GetNodeClient()
    );
    auto createWalletResponse = pWalletManager->InitializeNewWallet(
        CreateWalletCriteria("TestUser", "password", 24)
    );

    RPC::Request request = RPC::Request::BuildRequest("check_version");

    auto pClient = std::make_shared<HttpRpcClient>();
    auto response = pClient->Invoke(
        "127.0.0.1",
        "/v2/foreign",
        createWalletResponse.GetListenerPort(),
        request
    );

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    auto result = CheckVersionResponse::FromJSON(resultOpt.value());

    REQUIRE(result.GetForeignApiVersion() == 2);
    REQUIRE(result.GetSupportedSlateVersions() == std::vector<uint64_t>{ 2, 3 });
}