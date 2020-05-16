#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>

#include <TestServer.h>
#include <TestWalletServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: create_wallet - 24 words")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    RPC::Request request = RPC::Request::BuildRequest("create_wallet", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    std::string sessionToken64 = resultOpt->get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());

    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string walletSeed = resultOpt->get("wallet_seed", Json::Value()).asString();
    REQUIRE(StringUtil::Split(walletSeed, " ").size() == numWords);
}

TEST_CASE("API: create_wallet - 12 words")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    const uint8_t numWords = 12;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    RPC::Request request = RPC::Request::BuildRequest("create_wallet", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto resultOpt = response.GetResult();
    REQUIRE(resultOpt.has_value());

    std::string sessionToken64 = resultOpt->get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());

    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string walletSeed = resultOpt->get("wallet_seed", Json::Value()).asString();
    REQUIRE(StringUtil::Split(walletSeed, " ").size() == numWords);
}

TEST_CASE("API: create_wallet - User already exists")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    pTestWalletServer->CreateUser("david", "password");

    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    RPC::Request request = RPC::Request::BuildRequest("create_wallet", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto errorOpt = response.GetError();
    REQUIRE(errorOpt.has_value());

    REQUIRE(errorOpt->GetCode() == RPC::Errors::USER_ALREADY_EXISTS.GetCode());
}

TEST_CASE("API: create_wallet - Password empty")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "";
    paramsJson["num_seed_words"] = numWords;

    RPC::Request request = RPC::Request::BuildRequest("create_wallet", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto errorOpt = response.GetError();
    REQUIRE(errorOpt.has_value());

    REQUIRE(errorOpt->GetCode() == RPC::Errors::PASSWORD_CRITERIA_NOT_MET.GetCode());
}

TEST_CASE("API: create_wallet - Invalid num_seed_words")
{
    TestServer::Ptr pTestServer = TestServer::Create();
    TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

    const uint8_t numWords = 17;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    RPC::Request request = RPC::Request::BuildRequest("create_wallet", paramsJson);

    auto response = HttpRpcClient().Invoke("127.0.0.1", "/v2", pTestWalletServer->GetOwnerPort(), request);

    auto errorOpt = response.GetError();
    REQUIRE(errorOpt.has_value());

    REQUIRE(errorOpt->GetCode() == RPC::Errors::INVALID_NUM_SEED_WORDS.GetCode());
}