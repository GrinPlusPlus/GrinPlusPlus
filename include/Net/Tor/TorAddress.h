#pragma once

#include <string>
#include <Crypto/Models/ed25519_public_key.h>

class TorAddress
{
public:
	TorAddress(const std::string& address, const ed25519_public_key_t& publicKey, const uint8_t version)
		: m_address(address), m_publicKey(publicKey), m_version(version) { }

	const std::string& ToString() const noexcept { return m_address; }
	const ed25519_public_key_t& GetPublicKey() const noexcept { return m_publicKey; }

	bool operator==(const TorAddress& rhs) const noexcept { return m_address == rhs.m_address; }
	bool operator!=(const TorAddress& rhs) const noexcept { return m_address != rhs.m_address; }
	bool operator<(const TorAddress& rhs) const noexcept { return m_address < rhs.m_address; }
	bool operator<=(const TorAddress& rhs) const noexcept { return m_address <= rhs.m_address; }

private:
	std::string m_address;
	ed25519_public_key_t m_publicKey;
	uint8_t m_version;
};