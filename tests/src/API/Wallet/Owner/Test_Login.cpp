#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>

#include <TestServer.h>
#include <TestWalletServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: login")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    pTestWalletServer->CreateUser("David", "P@ssw0rd123!");

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";

    RPC::Request request = RPC::Request::BuildRequest("login", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    std::string sessionToken64 = resultOpt.value().get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());
    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string torAddress = resultOpt.value().get("tor_address", Json::Value()).asString();
    REQUIRE(TorAddressParser().Parse(torAddress).has_value());
}

TEST_CASE("API: login - User doesn't exist")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";

    RPC::Request request = RPC::Request::BuildRequest("login", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto errorOpt = response.GetError();
    REQUIRE(errorOpt.has_value());

    REQUIRE(errorOpt.value().GetCode() == RPC::Errors::USER_DOESNT_EXIST.GetCode());
}

TEST_CASE("API: login - Invalid Password")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    pTestWalletServer->CreateUser("David", "P@ssw0rd123!");

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "password";

    RPC::Request request = RPC::Request::BuildRequest("login", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto errorOpt = response.GetError();
    REQUIRE(errorOpt.has_value());

    REQUIRE(errorOpt.value().GetCode() == RPC::Errors::INVALID_PASSWORD.GetCode());
}