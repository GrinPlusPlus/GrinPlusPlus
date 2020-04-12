#include <catch.hpp>

#include <API/Wallet/Owner/OwnerServer.h>
#include <Net/Clients/RPC/RPCClient.h>

#include <TestServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: create_wallet")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    IWalletManagerPtr pWalletManager = WalletAPI::CreateWalletManager(*pTestServer->GetConfig(), pTestServer->GetNodeClient());

    auto pOwnerServer = OwnerServer::Create(*pTestServer->GetConfig(), pWalletManager);
    const uint16_t portNumber = 3421;
    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    RPC::Request request = RPC::Request::BuildRequest("create_wallet", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", portNumber, request);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    std::string sessionToken64 = resultOpt.value().get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());

    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string walletSeed = resultOpt.value().get("wallet_seed", Json::Value()).asString();
    REQUIRE(StringUtil::Split(walletSeed, " ").size() == numWords);
}