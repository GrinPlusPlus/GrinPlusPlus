#pragma once

#include <Crypto/Models/BigInteger.h>
#include <Core/Traits/Printable.h>

#define ED25519_PUBKEY_LEN 32

/** An Ed25519 public key */
struct ed25519_public_key_t : public Traits::IPrintable
{
	ed25519_public_key_t() = default;
	ed25519_public_key_t(CBigInteger<32>&& bytes_) : bytes(std::move(bytes_)) { }
	ed25519_public_key_t(const CBigInteger<32>& bytes_) : bytes(bytes_) { }

	bool operator==(const ed25519_public_key_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const ed25519_public_key_t& rhs) const { return bytes != rhs.bytes; }

	uint8_t* data() noexcept { return bytes.data(); }
	const uint8_t* data() const noexcept { return bytes.data(); }
	const std::vector<uint8_t>& vec() const noexcept { return bytes.GetData(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return vec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return vec().cend(); }

	std::string Format() const final { return bytes.ToHex(); }

	CBigInteger<32> bytes;
};