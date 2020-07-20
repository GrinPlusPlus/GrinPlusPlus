#pragma once

#include <Crypto/ED25519.h>
#include <Crypto/X25519.h>
#include <sodium/crypto_sign_ed25519.h>
#include <cassert>

class Curve25519
{
public:
    static x25519_public_key_t ToX25519(const ed25519_public_key_t& edwards_pubkey)
    {
        x25519_public_key_t montgomery_pubkey;
        const int result = crypto_sign_ed25519_pk_to_curve25519(
            montgomery_pubkey.data(),
            edwards_pubkey.data()
        );
        if (result != 0) {
            throw std::exception();
        }

        return montgomery_pubkey;
    }

    static x25519_secret_key_t ToX25519(const ed25519_secret_key_t& edwards_seckey)
    {
        x25519_secret_key_t montgomery_seckey;
        const int result = crypto_sign_ed25519_sk_to_curve25519(
            montgomery_seckey.data(),
            edwards_seckey.data()
        );
        if (result != 0) {
            throw std::exception();
        }

        return montgomery_seckey;
    }

    static x25519_keypair_t ToX25519(const ed25519_keypair_t& edwards_keypair)
    {
        x25519_keypair_t x25519_keypair;
        x25519_keypair.pubkey = ToX25519(edwards_keypair.public_key);
        x25519_keypair.seckey = ToX25519(edwards_keypair.secret_key);
        return x25519_keypair;
    }
};