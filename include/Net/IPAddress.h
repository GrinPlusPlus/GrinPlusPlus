#pragma once

#include <Common/Compat.h>
#include <Core/Traits/Printable.h>
#include <Core/Traits/Serializable.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>

#include <asio.hpp>
//#include <asio/ip/address.hpp>

class IPAddress : public Traits::IPrintable, public Traits::ISerializable
{
public:
    //
    // Constructors
    //
    IPAddress(asio::ip::address&& address) : m_address(std::move(address)) { }
    IPAddress(const asio::ip::address& address) : m_address(address) { }
    IPAddress() = default;
    IPAddress(const IPAddress& other) = default;
    IPAddress(IPAddress&& other) noexcept = default;

    static IPAddress CreateV4(const std::array<uint8_t, 4>& bytes)
    {
        return IPAddress(asio::ip::make_address_v4(bytes));
    }

    static IPAddress CreateV6(const std::array<uint8_t, 16>& bytes)
    {
        return IPAddress(asio::ip::make_address_v6(bytes));
    }

    static IPAddress Parse(const std::string& addressStr)
    {
        asio::error_code ec;
        asio::ip::address address = asio::ip::make_address(addressStr, ec);
        if (ec)
        {
            throw DESERIALIZATION_EXCEPTION_F("IP address failed to parse with error: {}", ec.value());
        }

        return IPAddress(std::move(address));
    }

    //
    // Destructor
    //
    virtual ~IPAddress() = default;

    //
    // Operators
    //
    IPAddress& operator=(const IPAddress& other) = default;
    IPAddress& operator=(IPAddress&& other) noexcept = default;
    bool operator==(const IPAddress& rhs) const { return m_address == rhs.m_address; }
    bool operator<(const IPAddress& rhs) const { return m_address < rhs.m_address; }

    //
    // Getters
    //
    const asio::ip::address& GetAddress() const noexcept { return m_address; }
    bool IsV4() const noexcept { return m_address.is_v4(); }
    bool IsV6() const noexcept { return m_address.is_v6(); }
    bool IsLocalhost() const noexcept { return m_address.is_loopback(); }
    std::string Format() const final { return m_address.to_string(); }

    //
    // Serialization/Deserialization
    //
    void Serialize(Serializer& serializer) const final
    {
        if (IsV4())
        {
            serializer.Append<uint8_t>(0);
            serializer.Append(m_address.to_v4().to_bytes());
        }
        else
        {
            serializer.Append<uint8_t>(1);
            serializer.Append(m_address.to_v6().to_bytes());
        }
    }

    static IPAddress Deserialize(ByteBuffer& byteBuffer)
    {
        const uint8_t ipAddressFamily = byteBuffer.ReadU8();
        if (ipAddressFamily == 0)
        {
            return CreateV4(byteBuffer.ReadArray<4>());
        }
        else if (ipAddressFamily == 1)
        {
            return CreateV6(byteBuffer.ReadArray<16>());
        }
        else
        {
            throw DESERIALIZATION_EXCEPTION_F("Invalid IP address family: {}", ipAddressFamily);
        }
    }

private:
    asio::ip::address m_address;
};

namespace std
{
    template<>
    struct hash<IPAddress>
    {
        size_t operator()(const IPAddress& address) const
        {
            asio::ip::address_v4 v4 = address.IsV4() ?
                address.GetAddress().to_v4() :
                address.GetAddress().to_v6().to_v4();
            return v4.to_uint();
        }
    };
}