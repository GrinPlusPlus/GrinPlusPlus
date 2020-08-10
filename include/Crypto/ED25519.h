#pragma once

#include <Crypto/Models/ed25519_public_key.h>
#include <Crypto/Models/ed25519_secret_key.h>
#include <Crypto/Models/ed25519_keypair.h>
#include <Crypto/Models/ed25519_signature.h>

class ED25519
{
public:
	static bool IsValid(const ed25519_public_key_t& pubkey);

	static ed25519_public_key_t ToPubKey(const std::vector<uint8_t>& bytes);
	static ed25519_keypair_t CalculateKeypair(const SecretKey& seed);
	static ed25519_public_key_t CalculatePubKey(const ed25519_secret_key_t& secretKey);
	static SecretKey64 CalculateTorKey(const ed25519_secret_key_t& secretKey);

	static bool VerifySignature(
		const ed25519_public_key_t& publicKey,
		const ed25519_signature_t& signature,
		const std::vector<uint8_t>& message
	);
	static ed25519_signature_t Sign(
		const ed25519_secret_key_t& secretKey,
		const std::vector<uint8_t>& message
	);
};