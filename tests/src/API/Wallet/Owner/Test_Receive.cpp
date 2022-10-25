#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>
#include <API/Wallet/Owner/Models/SendResponse.h>

#include <Core/Global.h>
#include <TestServer.h>
#include <TestChain.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: Slatepack - Generate new Address on Receive")
{
    REQUIRE(Global::GetConfig().ShouldReuseAddresses() == true);
    Global::GetConfig().ShouldReuseAddresses(false);
    REQUIRE(Global::GetConfig().ShouldReuseAddresses() == false);
	
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();
    	
    auto pSenderWallet = pTestServer->CreateUser("Alice", "P@ssw0rd123!", UseTor::YES).wallet;
    auto pReceiverWallet = pTestServer->CreateUser("Bob", "P@ssw0rd123!", UseTor::YES).wallet;

    TestChain chain(pTestServer->GetBlockChain());
    chain.MineChain(pSenderWallet, 30);

    // sender address before receiving
    std::string s_addressBefore = pSenderWallet->GetSlatepackAddress().ToString();
	// receiver address before receiving
    std::string r_AddressBefore = pReceiverWallet->GetSlatepackAddress().ToString();
	
    // Send
    SendCriteria criteria(
        pSenderWallet->GetToken(),
        1'000'000'000,
        1'000'000,
        (uint8_t)1,
        SelectionStrategyDTO(ESelectionStrategy::SMALLEST, {}),
        std::make_optional<std::string>(r_AddressBefore),
        std::nullopt
    );

    auto response_result = pTestServer->InvokeOwnerRPC("send", criteria.ToJSON()).GetResult();
    REQUIRE(response_result.has_value());

    SendResponse response = SendResponse::FromJSON(response_result.value());
    REQUIRE(response.GetStatus() == SendResponse::EStatus::FINALIZED);

    // Unpack slatepack
    SlatepackMessage decrypted = pReceiverWallet->GetWallet().Read()->DecryptSlatepack(response.GetArmoredSlate());

    ByteBuffer deserializer(decrypted.m_payload);
    REQUIRE(Slate::Deserialize(deserializer) == response.GetSlate());

    // sender address after receiving should be the same
    REQUIRE(s_addressBefore == pSenderWallet->GetSlatepackAddress().ToString());

    // receiver address after receiving should be different
    std::string newAddress = pReceiverWallet->GetSlatepackAddress().ToString();
    REQUIRE(r_AddressBefore != newAddress);

    // Send
    SendCriteria criteria2(
        pSenderWallet->GetToken(),
        1'000'000'000,
        1'000'000,
        (uint8_t)1,
        SelectionStrategyDTO(ESelectionStrategy::SMALLEST, {}),
        std::make_optional<std::string>(newAddress),
        std::nullopt
    );
    response_result = pTestServer->InvokeOwnerRPC("send", criteria2.ToJSON()).GetResult();
    REQUIRE(response_result.has_value());
    response = SendResponse::FromJSON(response_result.value());
    REQUIRE(response.GetStatus() == SendResponse::EStatus::FINALIZED);
    // receiver address after receiving should be different
    REQUIRE(newAddress != pReceiverWallet->GetSlatepackAddress().ToString());

    REQUIRE(Global::GetConfig().ShouldReuseAddresses() == false);
    Global::GetConfig().ShouldReuseAddresses(true);
    REQUIRE(Global::GetConfig().ShouldReuseAddresses() == true);

    newAddress = pReceiverWallet->GetSlatepackAddress().ToString();
    // Send
    SendCriteria criteria3(
        pSenderWallet->GetToken(),
        1'000'000'000,
        1'000'000,
        (uint8_t)1,
        SelectionStrategyDTO(ESelectionStrategy::SMALLEST, {}),
        std::make_optional<std::string>(newAddress),
        std::nullopt
    );
    response_result = pTestServer->InvokeOwnerRPC("send", criteria3.ToJSON()).GetResult();
    REQUIRE(response_result.has_value());
    response = SendResponse::FromJSON(response_result.value());
    REQUIRE(response.GetStatus() == SendResponse::EStatus::FINALIZED);
    // receiver address after receiving should be the same now
    REQUIRE(newAddress == pReceiverWallet->GetSlatepackAddress().ToString());
}
