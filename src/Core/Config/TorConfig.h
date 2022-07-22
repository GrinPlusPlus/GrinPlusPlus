#pragma once

#include "ConfigProps.h"

#include <json/json.h>

#include <filesystem.h>

#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>

class TorConfig
{
public:
	bool IsEnabled() const noexcept { return true; }

	// The "SocksPort" https://2019.www.torproject.org/docs/tor-manual.html.en#SocksPort
	uint16_t GetSocksPort() const noexcept { return m_socksPort; }

	// The "ControlPort" https://2019.www.torproject.org/docs/tor-manual.html.en#ControlPort
	uint16_t GetControlPort() const noexcept { return m_controlPort; }

	// The pre-hashed "ControlPassword" https://2019.www.torproject.org/docs/tor-manual.html.en#HashedControlPassword
	const std::string& GetControlPassword() const noexcept { return m_password; }

	// The "HashedControlPassword" https://2019.www.torproject.org/docs/tor-manual.html.en#HashedControlPassword
	const std::string& GetHashedControlPassword() const noexcept { return m_hashedPassword; }

	const fs::path& GetTorDataPath() const noexcept { return m_torDataPath; }

	const fs::path& GetTorrcPath() const noexcept { return m_torrcPath; }

	const std::string ReadTorrcFile() const noexcept
	{
		std::ifstream torrc(m_torrcPath);
		std::stringstream buffer;
		buffer << torrc.rdbuf();
		return buffer.str();
	}
	
	void AddTorBridge(std::string bridge) const noexcept
	{
		std::ofstream configFile(m_torrcPath, std::ios_base::app | std::ios_base::out);
		configFile << "Bridge " , bridge;
	}

	void ClearTorBridges() const noexcept
	{
		std::ifstream configFile(m_torrcPath);

		if (!configFile.is_open())
		{
			return;
		}
		std::ofstream newConfigFile;
		newConfigFile.open("temp.torrc", std::ofstream::out);

		std::string line;
		while (std::getline(configFile,line))
		{
			if (line.find("ClientTransportPlugin") == std::string::npos) {
				continue;
			}
			if (line.find("Bridge") == std::string::npos) {
				continue;
			}
			
			newConfigFile << line;
		}
		newConfigFile.close();
		configFile.close();
		remove(m_torrcPath);
		rename("temp.torrc", m_torrcPath);
	}

	//
	// Constructor
	//
	TorConfig(const Json::Value& json, const fs::path& torDataPath)
	{
		m_socksPort = 3422;
		m_controlPort = 3423;
		m_password = "MyPassword";
		m_hashedPassword = "16:906248AB51F939ED605CE9937D3B1FDE65DEB4098A889B2A07AC221D8F";
		m_torDataPath = torDataPath;
		m_torrcPath = torDataPath / ".torrc";
		fs::create_directories(torDataPath);
		if (!fs::exists(m_torrcPath)) std::ofstream file(m_torrcPath);

		if (json.isMember(ConfigProps::Tor::TOR))
		{
			const Json::Value& torJSON = json[ConfigProps::Tor::TOR];

			m_socksPort = (uint16_t)torJSON.get(ConfigProps::Tor::SOCKS_PORT, m_socksPort).asUInt();
			m_controlPort = (uint16_t)torJSON.get(ConfigProps::Tor::CONTROL_PORT, m_controlPort).asUInt();

			if (torJSON.isMember(ConfigProps::Tor::PASSWORD) && torJSON.isMember(ConfigProps::Tor::HASHED_PASSWORD))
			{
				m_password = torJSON[ConfigProps::Tor::PASSWORD].asCString();
				m_hashedPassword = torJSON[ConfigProps::Tor::HASHED_PASSWORD].asCString();
			}
		}
	}

	TorConfig(const uint16_t socksPort, const uint16_t controlPort, const fs::path& torDataPath)
		: m_socksPort(socksPort),
		m_controlPort(controlPort),
		m_password("MyPassword"),
		m_hashedPassword("16:906248AB51F939ED605CE9937D3B1FDE65DEB4098A889B2A07AC221D8F"),
		m_torDataPath(torDataPath)
	{

	}

private:
	uint16_t m_socksPort;
	uint16_t m_controlPort;
	std::string m_password;
	std::string m_hashedPassword;
	fs::path m_torDataPath;
	fs::path m_torrcPath;
};