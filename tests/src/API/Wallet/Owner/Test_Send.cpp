#include <catch.hpp>

#include <Net/Clients/RPC/RPCClient.h>
#include <API/Wallet/Owner/Models/SendResponse.h>

#include <TestServer.h>
#include <TestChain.h>
#include <Comparators/JsonComparator.h>
#include <optional>

TEST_CASE("API: send - Slatepack")
{
    TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

    auto pSenderWallet = pTestServer->CreateUser("Alice", "P@ssw0rd123!", UseTor::YES).wallet;
    auto pReceiverWallet = pTestServer->CreateUser("Bob", "P@ssw0rd123!", UseTor::NO).wallet;

    TestChain chain(pTestServer->GetBlockChain());
    chain.MineChain(pSenderWallet, 30);

    // Send
    SendCriteria criteria(
        pSenderWallet->GetToken(),
        1'000'000'000,
        1'000'000,
        (uint8_t)1,
        SelectionStrategyDTO(ESelectionStrategy::SMALLEST, {}),
        std::make_optional<std::string>(pReceiverWallet->GetSlatepackAddress().ToString()),
        std::nullopt
    );

    auto response_result = pTestServer->InvokeOwnerRPC("send", criteria.ToJSON()).GetResult();
    REQUIRE(response_result.has_value());

    SendResponse response = SendResponse::FromJSON(response_result.value());
    REQUIRE(response.GetStatus() == SendResponse::EStatus::SENT);

    // Unpack slatepack
    SlatepackMessage decrypted = pReceiverWallet->GetWallet().Read()->DecryptSlatepack(response.GetArmoredSlate());

    ByteBuffer deserializer(decrypted.m_payload);
    REQUIRE(Slate::Deserialize(deserializer) == response.GetSlate());
}
