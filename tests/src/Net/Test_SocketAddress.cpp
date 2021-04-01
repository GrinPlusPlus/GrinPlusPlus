#include <catch.hpp>

#include <Net/SocketAddress.h>

TEST_CASE("SocketAddress")
{
    //
    // V4 Serialization
    //
    uint16_t port = 512;
    std::array<uint8_t, 4> ipv4_bytes{ 0x10, 0x20, 0x30, 0x40 };
    SocketAddress ipv4 = SocketAddress::CreateV4(ipv4_bytes, port);
    REQUIRE(ipv4.Serialized() == std::vector<uint8_t>{ 0x00, 0x10, 0x20, 0x30, 0x40, 0x02, 0x00 });
    REQUIRE(ByteBuffer(ipv4.Serialized()).Read<SocketAddress>() == ipv4);

    //
    // V6 Serialization
    //
    std::array<uint8_t, 16> ipv6_bytes{ 0x05, 0x10, 0x15, 0x20, 0x25, 0x30, 0x35, 0x40, 0x45, 0x50, 0x55, 0x60, 0x65, 0x70, 0x75, 0x80 };
    SocketAddress ipv6 = SocketAddress::CreateV6(ipv6_bytes, port);
    REQUIRE(ipv6.Serialized() == std::vector<uint8_t>{ 0x01, 0x05, 0x10, 0x15, 0x20, 0x25, 0x30, 0x35, 0x40, 0x45, 0x50, 0x55, 0x60, 0x65, 0x70, 0x75, 0x80, 0x02, 0x00 });
    REQUIRE(ByteBuffer(ipv6.Serialized()).Read<SocketAddress>() == ipv6);
}