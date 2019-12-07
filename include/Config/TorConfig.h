#pragma once

#include <stdint.h>
#include <string>

class TorConfig
{
public:
	TorConfig(const uint16_t socksPort, const uint16_t controlPort, const std::string& password, const std::string& hashedPassword)
		: m_socksPort(socksPort), m_controlPort(controlPort), m_password(password), m_hashedPassword(hashedPassword)
	{

	}

	// The "SocksPort" https://2019.www.torproject.org/docs/tor-manual.html.en#SocksPort
	inline const uint16_t GetSocksPort() const { return m_socksPort; }

	// The "ControlPort" https://2019.www.torproject.org/docs/tor-manual.html.en#ControlPort
	inline const uint16_t GetControlPort() const { return m_controlPort; }

	// The pre-hashed "ControlPassword" https://2019.www.torproject.org/docs/tor-manual.html.en#HashedControlPassword
	inline const std::string& GetControlPassword() const { return m_password; }

	// The "HashedControlPassword" https://2019.www.torproject.org/docs/tor-manual.html.en#HashedControlPassword
	inline const std::string& GetHashedControlPassword() const { return m_hashedPassword; }

private:
	uint16_t m_socksPort;
	uint16_t m_controlPort;
	std::string m_password;
	std::string m_hashedPassword;
};