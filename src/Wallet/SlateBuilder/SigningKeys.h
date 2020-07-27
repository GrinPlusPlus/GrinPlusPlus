#pragma once

#include <Crypto/SecretKey.h>
#include <Crypto/PublicKey.h>

struct SigningKeys
{
    SecretKey secret_key;
    PublicKey public_key;
    SecretKey secret_nonce;
    PublicKey public_nonce;
};