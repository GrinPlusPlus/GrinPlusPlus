#pragma once

#include <Crypto/Models/SecretKey.h>
#include <Crypto/Models/PublicKey.h>

struct SigningKeys
{
    SecretKey secret_key;
    PublicKey public_key;
    SecretKey secret_nonce;
    PublicKey public_nonce;
};