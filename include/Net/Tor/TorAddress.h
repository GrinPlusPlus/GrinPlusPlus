#pragma once

#include <string>
#include <optional>
#include <Crypto/ED25519.h>

class TorAddress
{
public:
	TorAddress(const std::string& address, const ed25519_public_key_t& publicKey, const uint8_t version)
		: m_address(address), m_publicKey(publicKey), m_version(version)
	{
	
	}

	inline const std::string& ToString() const { return m_address; }

private:
	std::string m_address;
	ed25519_public_key_t m_publicKey;
	uint8_t m_version;
};