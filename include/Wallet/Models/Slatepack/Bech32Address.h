#pragma once

#include <bitcoin/bech32.h>

class Bech32Address
{
public:
    Bech32Address(const std::string& hrc, const std::vector<uint8_t>& data)
        : m_hrc(hrc), m_data(data) { }

    static Bech32Address Parse(const std::string& address)
    {
        std::pair<std::string, std::vector<uint8_t>> decoded = bech32::Decode(address);
        if (decoded.first.empty() || decoded.second.empty()) {
            throw std::exception();
        }

        return Bech32Address(decoded.first, FromBase32(decoded.second));
    }

    std::string ToString() const noexcept { return bech32::Encode(m_hrc, ToBase32(m_data)); }
    const std::string& GetHRC() const noexcept { return m_hrc; }
    const std::vector<uint8_t>& GetData() const noexcept { return m_data; }

private:
    static std::vector<uint8_t> FromBase32(const std::vector<uint8_t>& data)
    {
        uint32_t acc = 0;
        uint32_t bits = 0;
        uint32_t maxv = 255;
        
        std::vector<uint8_t> ret;

        for (const uint8_t value : data)
        {
            uint32_t v = (uint32_t)value;
            if (value > 31) {
                throw std::exception();
            }

            acc = (acc << 5) | v;
            bits += 5;
            while (bits >= 8) {
                bits -= 8;
                ret.push_back((uint8_t)((acc >> bits) & maxv));
            }
        }

        if (bits >= 5 || ((acc << (8 - bits)) & maxv) != 0) {
            throw std::exception();
        }
        
        return ret;
    }

    static std::vector<uint8_t> ToBase32(const std::vector<uint8_t>& data)
    {
        std::vector<uint8_t> ret;

        // Amount of bits left over from last round, stored in buffer.
        uint32_t buffer_bits = 0;
        // Holds all unwritten bits left over from last round. The bits are stored beginning from
        // the most significant bit. E.g. if buffer_bits=3, then the byte with bits a, b and c will
        // look as follows: [a, b, c, 0, 0, 0, 0, 0]
        uint8_t buffer = 0;

        for (const uint8_t b : data) {
            // Write first u5 if we have to write two u5s this round. That only happens if the
            // buffer holds too many bits, so we don't have to combine buffer bits with new bits
            // from this rounds byte.
            if (buffer_bits >= 5) {
                ret.push_back((buffer & 0b11111000) >> 3);
                buffer = buffer << 5;
                buffer_bits -= 5;
            }

            // Combine all bits from buffer with enough bits from this rounds byte so that they fill
            // a u5. Save reamining bits from byte to buffer.
            uint8_t from_buffer = buffer >> 3;
            uint8_t from_byte = b >> (3 + buffer_bits); // buffer_bits <= 4

            ret.push_back(from_buffer | from_byte);
            buffer = b << (5 - buffer_bits);
            buffer_bits = 3 + buffer_bits;
        }

        // There can be at most two u5s left in the buffer after processing all bytes, write them.
        if (buffer_bits >= 5) {
            ret.push_back((buffer & 0b11111000) >> 3);
            buffer = buffer << 5;
            buffer_bits -= 5;
        }

        if (buffer_bits != 0) {
            ret.push_back(buffer >> 3);
        }

        return ret;
    }

    std::string m_hrc;
    std::vector<uint8_t> m_data;
};