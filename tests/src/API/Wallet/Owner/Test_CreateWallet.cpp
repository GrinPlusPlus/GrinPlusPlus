#include <catch.hpp>

#include <TestServer.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: create_wallet - 24 words")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    auto response_result = pTestServer->InvokeOwnerRPC("create_wallet", paramsJson).GetResult();
    REQUIRE(response_result.has_value());

    std::string sessionToken64 = response_result.value().get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());

    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string walletSeed = response_result.value().get("wallet_seed", Json::Value()).asString();
    REQUIRE(StringUtil::Split(walletSeed, " ").size() == numWords);
}

TEST_CASE("API: create_wallet - 12 words")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    const uint8_t numWords = 12;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    auto response_result = pTestServer->InvokeOwnerRPC("create_wallet", paramsJson).GetResult();
    REQUIRE(response_result.has_value());

    std::string sessionToken64 = response_result.value().get("session_token", Json::Value()).asString();
    REQUIRE_FALSE(sessionToken64.empty());

    SessionToken token = SessionToken::FromBase64(sessionToken64);

    std::string walletSeed = response_result.value().get("wallet_seed", Json::Value()).asString();
    REQUIRE(StringUtil::Split(walletSeed, " ").size() == numWords);
}

TEST_CASE("API: create_wallet - User already exists")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    pTestServer->CreateUser("david", "password");

    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    auto response_error = pTestServer->InvokeOwnerRPC("create_wallet", paramsJson).GetError();
    REQUIRE(response_error.has_value());

    REQUIRE(response_error.value().GetCode() == RPC::Errors::USER_ALREADY_EXISTS.GetCode());
}

TEST_CASE("API: create_wallet - Password empty")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    const uint8_t numWords = 24;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "";
    paramsJson["num_seed_words"] = numWords;

    auto response_error = pTestServer->InvokeOwnerRPC("create_wallet", paramsJson).GetError();
    REQUIRE(response_error.has_value());
    REQUIRE(response_error.value().GetCode() == RPC::Errors::PASSWORD_CRITERIA_NOT_MET.GetCode());
}

TEST_CASE("API: create_wallet - Invalid num_seed_words")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    const uint8_t numWords = 17;

    Json::Value paramsJson;
    paramsJson["username"] = "David";
    paramsJson["password"] = "P@ssw0rd123!";
    paramsJson["num_seed_words"] = numWords;

    auto response_error = pTestServer->InvokeOwnerRPC("create_wallet", paramsJson).GetError();
    REQUIRE(response_error.has_value());
    REQUIRE(response_error.value().GetCode() == RPC::Errors::INVALID_NUM_SEED_WORDS.GetCode());
}