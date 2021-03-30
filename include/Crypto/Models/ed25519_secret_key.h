#pragma once

#include <Crypto/Models/BigInteger.h>
#include <Crypto/Models/SecretKey.h>

/*
 * An Ed25519 secret key
 * Encoding format: [32 bytes seed | 32 bytes public key]
 */
struct ed25519_secret_key_t
{
	ed25519_secret_key_t() = default;
	ed25519_secret_key_t(SecretKey64&& bytes_) : bytes(std::move(bytes_)) { }
	ed25519_secret_key_t(const SecretKey64& bytes_) : bytes(bytes_) { }

	bool operator==(const ed25519_secret_key_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const ed25519_secret_key_t& rhs) const { return bytes != rhs.bytes; }

	uint8_t* data() noexcept { return bytes.data(); }
	const uint8_t* data() const noexcept { return bytes.data(); }
	const std::vector<uint8_t>& vec() const noexcept { return bytes.GetVec(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return vec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return vec().cend(); }

	SecretKey64 bytes;
};