#pragma once

#include "ConfigProps.h"

#include <json/json.h>

#include <filesystem.h>

#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <iostream>

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
	
	void AddObfs4TorBridge(std::string bridge) const noexcept
	{
		if (bridge.empty()) return;
		if (!IsObfs4ConfigPresent()) AddObfs4Config();
		std::ofstream configFile(m_torrcPath, std::ios_base::app | std::ios_base::out);
		configFile << "Bridge " << bridge << "\n";
	}

	void ClearTorrcFile() const noexcept
	{
		std::ofstream ofs;
		ofs.open(m_torrcPath, std::ofstream::out | std::ofstream::trunc);
		ofs.close();
	}
	
	bool DisableObfsBridges() const noexcept
	{
		if (IsObfs4ConfigPresent()) RemoveObfs4Config();
		return IsObfs4ConfigPresent() == false;
	}
	
	bool EnableSnowflake() const noexcept
	{
		if (!IsSnowflakeConfigPresent()) AddSnowflakeConfig();
		return IsSnowflakeConfigPresent();
	}
	
	bool DisableSnowflake() const noexcept
	{
		if(IsSnowflakeConfigPresent()) RemoveSnowflakeConfig();
		return IsSnowflakeConfigPresent() == false;
	}

	bool IsTorBridgesEnabled() const noexcept
	{
		bool enabled = false;
		std::ifstream configFile(m_torrcPath);
		std::string line;
		while (std::getline(configFile, line))
		{
			if (line.find("UseBridges 1") != std::string::npos)
			{
				enabled = true;
				break;
			}
		}

		return enabled;
	}

	bool IsObfs4ConfigPresent() const noexcept
	{
		bool enabled = false;
		std::ifstream configFile(m_torrcPath);
		std::string line;
		while (std::getline(configFile, line)) 
		{
			if (line.find("ClientTransportPlugin obfs4 exec") != std::string::npos)
			{
				enabled = true;
				break;
			}
		}

		return enabled;
	}

	bool IsSnowflakeConfigPresent() const noexcept
	{
		bool enabled = false;
		std::ifstream configFile(m_torrcPath);
		std::string line;
		while (std::getline(configFile, line))
		{
			if (line.find("ClientTransportPlugin snowflake exec") != std::string::npos)
			{
				enabled = true;
				break;
			}
		}

		return enabled;
	}

	void DisableTorBridges() const noexcept
	{
		if (IsSnowflakeConfigPresent()) DisableSnowflake();
		else DisableObfsBridges();
	}

	std::vector<std::string> ListTorBridges() const noexcept
	{
		std::vector<std::string> bridges;
		std::ifstream configFile(m_torrcPath);
		std::string line;
		while (std::getline(configFile, line))
		{
			if (line.rfind("Bridge", 0) == 0)
			{
				bridges.push_back(line.substr(7)); // Bridges length = 6
			}
		}

		return bridges;
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

	void AddObfs4Config() const noexcept
	{
		AddRequiredHeadersTorBridges(false);
		return;
	}

	void AddSnowflakeConfig() const noexcept
	{
		AddRequiredHeadersTorBridges(true);
		return;
	}
	
	void AddRequiredHeadersTorBridges(const bool snowflake) const noexcept
	{
		std::ifstream configFile(m_torrcPath);
		std::string line;
		
		std::ofstream newConfigFile;
		newConfigFile.open("temp.torrc", std::ofstream::out | std::ofstream::trunc);
		newConfigFile << "UseBridges 1" << "\n";
		if (snowflake)
		{
#ifdef _WIN32
			fs::path snowflakeDir = (m_torDataPath / "PluggableTransports" / "snowflake-client.exe");
#else
			fs::path snowflakeDir = (m_torDataPath / "PluggableTransports" / "snowflake-client");
#endif
			newConfigFile << "ClientTransportPlugin snowflake exec " << snowflakeDir.u8string() << " ";
			newConfigFile << "-url https://snowflake-broker.torproject.net.global.prod.fastly.net/ ";
			newConfigFile << "-front cdn.sstatic.net ";
			newConfigFile << "-ice stun:stun.l.google.com:19302,stun:stun.voip.blackberry.com:3478,stun:stun.altar.com.pl:3478,stun:stun.antisip.com:3478,stun:stun.bluesip.net:3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel.com:3478,stun:stun.sonetel.net:3478,stun:stun.stunprotocol.org:3478,stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:3478\n";
			newConfigFile << "Bridge snowflake 192.0.2.3:1 2B280B23E1107BB62ABFC40DDCC8824814F80A72\n";
		}
		else
		{
#ifdef _WIN32
			fs::path obfs4Dir = (m_torDataPath / "PluggableTransports" / "obfs4proxy.exe");
#else
			fs::path obfs4Dir = (m_torDataPath / "PluggableTransports" / "obfs4proxy");
#endif
			newConfigFile << "ClientTransportPlugin obfs4 exec " << obfs4Dir.u8string() << "\n";
		}
		while (std::getline(configFile, line))
		{
			if (line.find("UseBridges 1") != std::string::npos ||
				line.find("ClientTransportPlugin") == std::string::npos ||
				line.find("Bridge") == std::string::npos) continue;

			newConfigFile << line << "\n";
		}
		
		newConfigFile.close();
		configFile.close();
		remove(m_torrcPath);
		rename("temp.torrc", m_torrcPath);
		
		return;
	}
	
	void RemoveObfs4Config() const noexcept
	{
		std::ifstream configFile(m_torrcPath);
		std::string line;

		std::ofstream newConfigFile;
		newConfigFile.open("temp.torrc", std::ofstream::out | std::ofstream::trunc);

		while (std::getline(configFile, line))
		{
			if (line.find("UseBridges 1") != std::string::npos ||
				line.find("ClientTransportPlugin obfs4") != std::string::npos ||
				line.find("Bridge obfs4") != std::string::npos)
			{
				newConfigFile << "";
			} else
			{
				newConfigFile << line << "\n";
			}
		}

		newConfigFile.close();
		configFile.close();
		remove(m_torrcPath);
		rename("temp.torrc", m_torrcPath);

		return;
	}
	
	void RemoveSnowflakeConfig() const noexcept
	{
		std::ifstream configFile(m_torrcPath);
		std::string line;
		
		std::ofstream newConfigFile;
		newConfigFile.open("temp.torrc", std::ofstream::out | std::ofstream::trunc);

		while (std::getline(configFile, line))
		{
			if (line.find("UseBridges 1") != std::string::npos ||
				line.find("ClientTransportPlugin snowflake") != std::string::npos || 
				line.find("Bridge snowflake") != std::string::npos)
			{
				newConfigFile << "";
			}
			else
			{
				newConfigFile << line << "\n";
			}
		}

		newConfigFile.close();
		configFile.close();
		remove(m_torrcPath);
		rename("temp.torrc", m_torrcPath);

		return;
	}
};