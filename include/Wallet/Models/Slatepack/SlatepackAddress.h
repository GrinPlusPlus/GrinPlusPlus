#pragma once

#include <Crypto/Curve25519.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Core/Serialization/Serializer.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorAddressParser.h>
#include <Wallet/Models/Slatepack/Bech32Address.h>

class SlatepackAddress
{
public:
    SlatepackAddress() : m_pubkey() { }
    SlatepackAddress(const ed25519_public_key_t& pubkey)
        : m_pubkey(pubkey) { }

    static SlatepackAddress Parse(const std::string& address)
    {
        std::vector<uint8_t> data = Bech32Address::Parse(address).GetData();
        ed25519_public_key_t pubkey = ED25519::ToPubKey(data);

        return SlatepackAddress(pubkey);
    }

    static SlatepackAddress Random()
    {
        SecureVector sec_key = RandomNumberGenerator().GenerateRandomBytes(64);
        return SlatepackAddress(ED25519::CalculatePubKey(CBigInteger<64>{ sec_key.data() }));
    }

    const ed25519_public_key_t& GetEdwardsPubKey() const noexcept { return m_pubkey; }
    x25519_public_key_t ToX25519PubKey() const { return Curve25519::ToX25519(m_pubkey); }

    TorAddress ToTorAddress() const noexcept { return TorAddressParser::FromPubKey(m_pubkey); }
    std::string ToString() const noexcept { return Bech32Address(HRC, m_pubkey.vec()).ToString(); }

    void Serialize(Serializer& serializer) const noexcept
    {
        std::string str = ToString();
        serializer.Append<uint8_t>((uint8_t)str.size());
        serializer.AppendFixedStr(str);
    }

private:
    ed25519_public_key_t m_pubkey;

    inline static const std::string HRC = "grin"; // TODO: "tgrin" for floonet
};