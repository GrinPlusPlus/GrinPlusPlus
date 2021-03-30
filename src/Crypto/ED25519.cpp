#include <Crypto/ED25519.h>

#include <sodium/crypto_sign.h>
#include <sodium/crypto_core_ed25519.h>
#include <sodium/crypto_sign_ed25519.h>
#include <Core/Exceptions/CryptoException.h>

ed25519_public_key_t ED25519::ToPubKey(const std::vector<uint8_t>& bytes)
{
	if (bytes.size() != ED25519_PUBKEY_LEN) {
		throw std::exception();
	}

	ed25519_public_key_t pubkey;
	pubkey.bytes = bytes;
	if (!IsValid(pubkey)) {
		throw std::exception();
	}

	return pubkey;
}

bool ED25519::IsValid(const ed25519_public_key_t& pubkey)
{
	return crypto_core_ed25519_is_valid_point(pubkey.data()) != 0;
}

ed25519_keypair_t ED25519::CalculateKeypair(const SecretKey& seed)
{
	ed25519_public_key_t public_key;
	ed25519_secret_key_t secret_key;
	const int status = crypto_sign_ed25519_seed_keypair(
		public_key.data(),
		secret_key.data(),
		seed.data()
	);
	if (status != 0) {
		throw CryptoException("ED25519::CalculateKeypair");
	}

	return ed25519_keypair_t(std::move(public_key), std::move(secret_key));
}

ed25519_public_key_t ED25519::CalculatePubKey(const ed25519_secret_key_t& secretKey)
{
	ed25519_public_key_t result;
	const int status = crypto_sign_ed25519_sk_to_pk(result.data(), secretKey.data());
	if (status != 0)
	{
		throw CryptoException("ED25519::CalculatePubKey");
	}

	return result;
}

bool ED25519::VerifySignature(
	const ed25519_public_key_t& publicKey,
	const ed25519_signature_t& signature,
	const std::vector<uint8_t>& message)
{
	std::vector<uint8_t> sig_and_message;
	sig_and_message.insert(sig_and_message.end(), signature.cbegin(), signature.cend());
	sig_and_message.insert(sig_and_message.end(), message.cbegin(), message.cend());
	std::vector<uint8_t> message_out(message.size());
	unsigned long long message_len = message_out.size();

	const int status = crypto_sign_open(
		message_out.data(),
		&message_len,
		sig_and_message.data(),
		sig_and_message.size(),
		publicKey.data()
	);

	return status == 0;
}

ed25519_signature_t ED25519::Sign(const ed25519_secret_key_t& secretKey, const std::vector<uint8_t>& message)
{
	std::vector<uint8_t> signature_bytes(64 + message.size());
	unsigned long long signature_size = signature_bytes.size();
	const int status = crypto_sign_ed25519(
		signature_bytes.data(),
		&signature_size,
		message.data(),
		message.size(),
		secretKey.data()
	);
	if (status != 0 || signature_size < 64)
	{
		throw CryptoException("ED25519::Sign");
	}

	return ed25519_signature_t(CBigInteger<64>{ std::vector<uint8_t>(signature_bytes.cbegin(), signature_bytes.cbegin() + 64) });
}

// Given an ed25519_secret_key_t, calculates the 32-byte secret scalar (a) and the PRF secret (RH).
// See: http://ffp4g1ylyit3jdyti1hqcvtb-wpengine.netdna-ssl.com/warner/files/2011/11/key-formats.png
SecretKey64 ED25519::CalculateTorKey(const ed25519_secret_key_t& secretKey)
{
	SecretKey64 torKey;
	crypto_hash_sha512(torKey.data(), secretKey.data(), 32);
	torKey[0] &= 248;
	torKey[31] &= 127;
	torKey[31] |= 64;
	return torKey;
}