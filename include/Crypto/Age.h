#pragma once

#include <Crypto/X25519.h>
#include <Crypto/SecretKey.h>
#include <Core/Serialization/Base64.h>

namespace age
{
    struct RecipientLine
    {
        x25519_public_key_t ephemeral_public_key;
        SecretKey encrypted_file_key;

        std::string Encode() const noexcept
        {
            return StringUtil::Format(
                "-> X25519 {}\n{}",
                Base64::EncodeUnpadded(ephemeral_public_key.bytes.GetData()),
                Base64::EncodeUnpadded(encrypted_file_key.GetVec())
            );
        }
    };

    struct Header
    {
        SecretKey mac;
        std::vector<RecipientLine> recipients;

        std::string Encode() const noexcept
        {
            std::string encoded_recipients = "";
            for (const RecipientLine& recipient : recipients) {
                encoded_recipients += recipient.Encode() + "\n";
            }

            return StringUtil::Format(
                "ge-encryption.org/v1\n{}--- {}\n",
                encoded_recipients,
                Base64::EncodeUnpadded(mac.GetVec())
            );
        }
    };

}

class Age
{
public:
    static std::vector<uint8_t> Encrypt(
        const std::vector<x25519_public_key_t>& recipients,
        const std::vector<uint8_t>& payload
    );

private:
    static std::vector<age::RecipientLine> BuildRecipientLines(
        const CBigInteger<16>& file_key,
        const std::vector<x25519_public_key_t>& recipients
    );

    static void StreamEncrypt(
        Serializer& serializer,
        const SecretKey& key,
        const std::vector<uint8_t>& data
    );
};