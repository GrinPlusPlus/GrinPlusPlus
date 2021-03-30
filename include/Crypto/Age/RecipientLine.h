#pragma once

#include <Crypto/KDF.h>
#include <Crypto/X25519.h>
#include <Crypto/Models/SecretKey.h>
#include <Common/Util/StringUtil.h>
#include <Core/Serialization/Base64.h>
#include <optional>

namespace age
{
    struct RecipientLine
    {
        x25519_public_key_t ephemeral_public_key;
        CBigInteger<32> encrypted_file_key;

        std::string Encode() const noexcept
        {
            return StringUtil::Format(
                "-> X25519 {}\n{}\n",
                Base64::EncodeUnpadded(ephemeral_public_key.bytes.GetData()),
                Base64::EncodeUnpadded(encrypted_file_key.GetData())
            );
        }

        static SecretKey CalcEncryptionKey(
            const SecretKey& shared_secret,
            const x25519_public_key_t& ephemeral_public_key,
            const x25519_public_key_t& public_key)
        {
            std::vector<uint8_t> salt;
            salt.insert(salt.end(), ephemeral_public_key.cbegin(), ephemeral_public_key.cend());
            salt.insert(salt.end(), public_key.cbegin(), public_key.cend());
            SecretKey enc_key = KDF::HKDF(
                std::make_optional(std::move(salt)),
                "age-encryption.org/v1/X25519",
                shared_secret.GetVec()
            );

            return enc_key;
        }
    };
}