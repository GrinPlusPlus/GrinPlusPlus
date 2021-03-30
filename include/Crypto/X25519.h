#pragma once

#include <Crypto/Models/SecretKey.h>
#include <sodium/crypto_kx.h>
#include <sodium/crypto_scalarmult.h>

/** An X25519 public key */
struct x25519_public_key_t
{
	x25519_public_key_t() = default;
	x25519_public_key_t(CBigInteger<32>&& bytes_) : bytes(std::move(bytes_)) { }
	x25519_public_key_t(const CBigInteger<32>& bytes_) : bytes(bytes_) { }

	bool operator==(const x25519_public_key_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const x25519_public_key_t& rhs) const { return bytes != rhs.bytes; }

	unsigned char* data() noexcept { return bytes.data(); }
	const unsigned char* data() const noexcept { return bytes.data(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return bytes.GetData().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return bytes.GetData().cend(); }

	CBigInteger<32> bytes;
};

/** An X25519 secret key */
struct x25519_secret_key_t
{
	x25519_secret_key_t() = default;
	x25519_secret_key_t(SecretKey&& bytes_) : bytes(std::move(bytes_)) { }
	x25519_secret_key_t(const SecretKey& bytes_) : bytes(bytes_) { }

	bool operator==(const x25519_secret_key_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const x25519_secret_key_t& rhs) const { return bytes != rhs.bytes; }

	unsigned char* data() noexcept { return bytes.data(); }
	const unsigned char* data() const noexcept { return bytes.data(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return bytes.GetVec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return bytes.GetVec().cend(); }

	SecretKey bytes;
};

struct x25519_keypair_t
{
	x25519_keypair_t() = default;
	x25519_keypair_t(x25519_secret_key_t&& seckey_, x25519_public_key_t&& pubkey_)
		: seckey(std::move(seckey_)), pubkey(std::move(pubkey_)) { }
	x25519_keypair_t(const x25519_secret_key_t& seckey_, const x25519_public_key_t& pubkey_)
		: seckey(seckey_), pubkey(pubkey_) { }

	x25519_secret_key_t seckey;
	x25519_public_key_t pubkey;
};

class X25519
{
public:
	static x25519_keypair_t GenerateEphemeralKeypair()
	{
		x25519_secret_key_t seckey;
		x25519_public_key_t pubkey;
		const int result = crypto_kx_keypair(pubkey.data(), seckey.data());
		if (result != 0) {
			throw std::exception();
		}

		return x25519_keypair_t(std::move(seckey), std::move(pubkey));
	}

	static SecretKey ECDH(const x25519_secret_key_t& seckey_alice, const x25519_public_key_t& pubkey_bob)
	{
		SecretKey shared_secret;
		const int result = crypto_scalarmult_curve25519(
			shared_secret.data(),
			seckey_alice.data(),
			pubkey_bob.data()
		);
		if (result != 0) {
			throw std::exception();
		}

		return shared_secret;
	}

	static x25519_public_key_t ToPubKey(const x25519_secret_key_t& secret)
	{
		x25519_public_key_t pubkey;
		const int result = crypto_scalarmult_curve25519_base(pubkey.data(), secret.data());
		if (result != 0) {
			throw std::exception();
		}

		return pubkey;
	}
};