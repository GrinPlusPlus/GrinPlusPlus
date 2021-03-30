#pragma once

#include <Crypto/Models/BigInteger.h>
#include <Core/Traits/Printable.h>

struct ed25519_signature_t : public Traits::IPrintable
{
	ed25519_signature_t() = default;
	ed25519_signature_t(const CBigInteger<64>& bytes_)
		: bytes(bytes_) { }
	ed25519_signature_t(CBigInteger<64>&& bytes_)
		: bytes(std::move(bytes_)) { }

	bool operator==(const ed25519_signature_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const ed25519_signature_t& rhs) const { return bytes != rhs.bytes; }

	uint8_t* data() noexcept { return bytes.data(); }
	const uint8_t* data() const noexcept { return bytes.data(); }
	const std::vector<uint8_t>& vec() const noexcept { return bytes.GetData(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return vec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return vec().cend(); }

	std::string ToHex() const { return bytes.ToHex(); }
	std::string Format() const final { return ToHex(); }

	CBigInteger<64> bytes;
};