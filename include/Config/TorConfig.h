#pragma once

#include <Config/ConfigProps.h>

#include <json/json.h>
#include <cstdint>
#include <string>

class TorConfig
{
public:
	// The "SocksPort" https://2019.www.torproject.org/docs/tor-manual.html.en#SocksPort
	uint16_t GetSocksPort() const { return m_socksPort; }

	// The "ControlPort" https://2019.www.torproject.org/docs/tor-manual.html.en#ControlPort
	uint16_t GetControlPort() const { return m_controlPort; }

	// The pre-hashed "ControlPassword" https://2019.www.torproject.org/docs/tor-manual.html.en#HashedControlPassword
	const std::string& GetControlPassword() const { return m_password; }

	// The "HashedControlPassword" https://2019.www.torproject.org/docs/tor-manual.html.en#HashedControlPassword
	const std::string& GetHashedControlPassword() const { return m_hashedPassword; }

	//
	// Constructor
	//
	TorConfig(const Json::Value& json)
	{
		m_socksPort = 3422;
		m_controlPort = 3423;
		m_password = "MyPassword";
		m_hashedPassword = "16:906248AB51F939ED605CE9937D3B1FDE65DEB4098A889B2A07AC221D8F";

		if (json.isMember(ConfigProps::Tor::TOR))
		{
			const Json::Value& torJSON = json[ConfigProps::Tor::TOR];

			if (torJSON.isMember(ConfigProps::Tor::SOCKS_PORT))
			{
				m_socksPort = (uint16_t)torJSON[ConfigProps::Tor::SOCKS_PORT].asUInt();
			}

			if (torJSON.isMember(ConfigProps::Tor::CONTROL_PORT))
			{
				m_controlPort = (uint16_t)torJSON[ConfigProps::Tor::CONTROL_PORT].asUInt();
			}

			if (torJSON.isMember(ConfigProps::Tor::PASSWORD) && torJSON.isMember(ConfigProps::Tor::HASHED_PASSWORD))
			{
				m_password = torJSON[ConfigProps::Tor::PASSWORD].asCString();
				m_hashedPassword = torJSON[ConfigProps::Tor::HASHED_PASSWORD].asCString();
			}
		}
	}

private:
	uint16_t m_socksPort;
	uint16_t m_controlPort;
	std::string m_password;
	std::string m_hashedPassword;
};