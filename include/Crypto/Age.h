#pragma once

#include <Crypto/Age/RecipientLine.h>
#include <Crypto/Age/Header.h>
#include <Crypto/X25519.h>
#include <Crypto/Models/SecretKey.h>
#include <Crypto/Models/Hash.h>
#include <Core/Serialization/Base64.h>
#include <regex>

class Age
{
public:
    static std::vector<uint8_t> Encrypt(
        const std::vector<x25519_public_key_t>& recipients,
        const std::vector<uint8_t>& payload
    );

    static std::vector<uint8_t> Decrypt(
        const x25519_keypair_t& decrypt_keypair,
        const std::vector<uint8_t>& payload
    );

private:
    static std::vector<age::RecipientLine> BuildRecipientLines(
        const CBigInteger<16>& file_key,
        const std::vector<x25519_public_key_t>& recipients
    );

    static std::vector<uint8_t> TryDecrypt(
        const x25519_keypair_t& decrypt_keypair,
        const age::RecipientLine& recipient_line,
        const CBigInteger<16>& file_key_nonce,
        const age::Header& header,
        const std::vector<uint8_t>& encrypted_payload
    );
};