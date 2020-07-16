#include <catch.hpp>

#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Wallet/Models/Slatepack/SlatepackMessage.h>

TEST_CASE("SlatepackAddress")
{
    std::string addressStr = "grin149xh54wumpmc9uc7u8k936kqhzakg5uhgkl503kghmfal2nnp6vsu5ff0c";
    SlatepackAddress address = SlatepackAddress::Parse(addressStr);

    REQUIRE(address.ToString() == addressStr);

    SlatepackMessage message;
    message.m_sender = address;
    message.m_mode = SlatepackMessage::ENCRYPTED;
    message.m_payload = CBigInteger<4>::FromHex("01020304").GetData();

    SlatepackAddress recipient1 = SlatepackAddress::Random();
    std::vector<uint8_t> encrypted = message.EncryptPayload({ recipient1 });

    REQUIRE(HexUtil::ConvertToHex(encrypted) == address.ToString());
}