#pragma once

#include <Net/Tor/TorAddress.h>
#include <Crypto/Models/ed25519_public_key.h>
#include <optional>

class TorAddressParser
{
public:
	static std::optional<TorAddress> Parse(const std::string& address);
	static TorAddress FromPubKey(const ed25519_public_key_t& pubkey);

private:
	static bool IsValid(
		const uint8_t version,
		const ed25519_public_key_t& pubkey,
		const std::vector<uint8_t>& checksum
	);

	static bool BuildChecksum(
		const ed25519_public_key_t& key,
		uint8_t version,
		std::vector<uint8_t>& checksum_out
	);
};