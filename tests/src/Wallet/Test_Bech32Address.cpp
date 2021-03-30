#include <catch.hpp>
#include <iostream>

#include <Crypto/CSPRNG.h>
#include <Crypto/Crypto.h>
#include <Crypto/ED25519.h>
#include <Crypto/Models/SecretKey.h>
#include <Wallet/Models/Slatepack/Bech32Address.h>

TEST_CASE("Bech32Address")
{
    uint8_t version = 1;
    ed25519_keypair_t ed25519_keypair = ED25519::CalculateKeypair(SecretKey{ CSPRNG::GenerateRandomBytes(32) });
    SecretKey excess = CSPRNG::GenerateRandomBytes(32);
    SecretKey nonce = CSPRNG::GenerateRandomBytes(32);

    PublicKey pub_excess = Crypto::CalculatePublicKey(excess);
    PublicKey pub_nonce = Crypto::CalculatePublicKey(nonce);

    std::vector<uint8_t> message;
    message.insert(message.end(), pub_excess.cbegin(), pub_excess.cend());
    message.insert(message.end(), pub_nonce.cbegin(), pub_nonce.cend());
    ed25519_signature_t signature = ED25519::Sign(ed25519_keypair.secret_key, message);

    std::vector<uint8_t> encoded;
    encoded.push_back(1);
    encoded.insert(encoded.end(), ed25519_keypair.public_key.cbegin(), ed25519_keypair.public_key.cend());
    encoded.insert(encoded.end(), pub_excess.cbegin(), pub_excess.cend());
    encoded.insert(encoded.end(), pub_nonce.cbegin(), pub_nonce.cend());
    encoded.insert(encoded.end(), signature.cbegin(), signature.cend());

    std::cout << "ed25519 pubkey: " << ed25519_keypair.public_key.Format() << std::endl;
    std::cout << "public excess: " << pub_excess.Format() << std::endl;
    std::cout << "public nonce: " << pub_nonce.Format() << std::endl;
    std::cout << "signature of excess & nonce: " << signature.Format() << std::endl;
    std::cout << "bech32 address: " << Bech32Address("grin", encoded).ToString() << std::endl;
}