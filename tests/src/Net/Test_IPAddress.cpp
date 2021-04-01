#include <catch.hpp>

#include <Net/IPAddress.h>

TEST_CASE("IPAddress")
{
    //
    // V4 Serialization
    //
    std::array<uint8_t, 4> ipv4_bytes{ 0x10, 0x20, 0x30, 0x40 };
    IPAddress ipv4 = IPAddress::CreateV4(ipv4_bytes);
    REQUIRE(ipv4.Serialized() == std::vector<uint8_t>{ 0x00, 0x10, 0x20, 0x30, 0x40 });
    REQUIRE(ByteBuffer(ipv4.Serialized()).Read<IPAddress>() == ipv4);

    //
    // V6 Serialization
    //
    std::array<uint8_t, 16> ipv6_bytes{ 0x05, 0x10, 0x15, 0x20, 0x25, 0x30, 0x35, 0x40, 0x45, 0x50, 0x55, 0x60, 0x65, 0x70, 0x75, 0x80 };
    IPAddress ipv6 = IPAddress::CreateV6(ipv6_bytes);
    REQUIRE(ipv6.Serialized() == std::vector<uint8_t>{ 0x01, 0x05, 0x10, 0x15, 0x20, 0x25, 0x30, 0x35, 0x40, 0x45, 0x50, 0x55, 0x60, 0x65, 0x70, 0x75, 0x80 });
    REQUIRE(ByteBuffer(ipv6.Serialized()).Read<IPAddress>() == ipv6);
}