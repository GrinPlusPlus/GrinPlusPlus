#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>

#include <TestServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: login")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    CreatedUser user = pTestServer->CreateUser("David", "P@ssw0rd123!");

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";

    std::this_thread::sleep_for(std::chrono::seconds(10));
    auto response_result = pTestServer->InvokeOwnerRPC("login", paramsJson).GetResult();
    REQUIRE(response_result.has_value());

    std::string sessionToken64 = response_result.value().get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());
    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string torAddress = response_result.value().get("tor_address", Json::Value()).asString();
    REQUIRE(TorAddressParser().Parse(torAddress).has_value());
}

TEST_CASE("API: login - User doesn't exist")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";

    auto response_error = pTestServer->InvokeOwnerRPC("login", paramsJson).GetError();
    REQUIRE(response_error.has_value());
    REQUIRE(response_error.value().GetCode() == RPC::Errors::USER_DOESNT_EXIST.GetCode());
}

TEST_CASE("API: login - Invalid Password")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    pTestServer->CreateUser("David", "P@ssw0rd123!");

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "password";

    auto response_error = pTestServer->InvokeOwnerRPC("login", paramsJson).GetError();
    REQUIRE(response_error.has_value());
    REQUIRE(response_error.value().GetCode() == RPC::Errors::INVALID_PASSWORD.GetCode());
}