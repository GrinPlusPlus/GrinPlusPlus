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
	
	void AddTorBridge(std::string bridge) const noexcept
	{
		AddRequiredHeadersTorBridges(false);
		std::ofstream configFile(m_torrcPath, std::ios_base::app | std::ios_base::out);
		configFile << "Bridge " << bridge << "\n";
	}

	void ClearTorrcFile() const noexcept
	{
		std::ofstream ofs;
		ofs.open(m_torrcPath, std::ofstream::out | std::ofstream::trunc);
		ofs.close();
	}


	void EnableSnowflake(bool enable) const noexcept
	{
		if (enable)
		{
			AddRequiredHeadersTorBridges(false);
		}
		else
		{
			RemoveSnowflakeConfig();
		}
		return ;
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

	bool IsObfs4Enabled() const noexcept
	{
		bool enabled = false;
		std::ifstream configFile(m_torrcPath);
		std::string line;
		while (std::getline(configFile, line)) 
		{
			if (line.find("ClientTransportPlugin obfs4") != std::string::npos)
			{
				enabled = true;
				break;
			}
		}

		return enabled;
	}

	bool IsSnowflakeEnabled() const noexcept
	{
		bool enabled = false;
		std::ifstream configFile(m_torrcPath);
		std::string line;
		while (std::getline(configFile, line))
		{
			if (line.find("ClientTransportPlugin snowflake") != std::string::npos)
			{
				enabled = true;
				break;
			}
		}

		return enabled;
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

	void AddRequiredHeadersTorBridges(const bool snowflake) const noexcept
	{
		std::ifstream configFile(m_torrcPath);
		std::string line;
		int i = 0;
		
		std::ofstream newConfigFile;
		newConfigFile.open("temp.torrc", std::ofstream::out);
		
		while (std::getline(configFile, line))
		{
			i += 1;
			
			if (i == 1)
			{
				if(line.find("UseBridges 1") == std::string::npos) newConfigFile << "UseBridges 1" << "\n";
			}
			else if (i == 2)
			{
				if (line.find("ClientTransportPlugin") == std::string::npos)
				{
					if(!snowflake)
					{
#ifdef _WIN32
						fs::path obfs4Dir = (m_torDataPath / "PluggableTransports" / "obfs4proxy.exe");
#else
						fs::path obfs4Dir = (m_torDataPath / "PluggableTransports" / "obfs4proxy");
#endif
						newConfigFile << "ClientTransportPlugin obfs4 exec " << obfs4Dir << "\n";
					}
					else if (snowflake)
					{
#ifdef _WIN32
						fs::path snowflakeDir = (m_torDataPath / "PluggableTransports" / "snowflake-client.exe");
#else
						fs::path snowflakeDir = (m_torDataPath / "PluggableTransports" / "snowflake-client");
#endif
						newConfigFile << "ClientTransportPlugin snowflake  exec " << snowflakeDir << " ";
						newConfigFile << "-url https://snowflake-broker.torproject.net.global.prod.fastly.net/ ";
						newConfigFile << "-front cdn.sstatic.net \\ \n";
						newConfigFile << "-ice stun:stun.l.google.com:19302,stun:stun.voip.blackberry.com:3478,stun:stun.altar.com.pl:3478,stun:stun.antisip.com:3478,stun:stun.bluesip.net:3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel.com:3478,stun:stun.sonetel.net:3478,stun:stun.stunprotocol.org:3478,stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:3478 \n";
						newConfigFile << "Bridge snowflake 192.0.2.3:1 \n";
					}
				}
			}		
			
			newConfigFile << line << "\n";
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
		newConfigFile.open("temp.torrc", std::ofstream::out);

		while (std::getline(configFile, line))
		{
			if (line.find("UseBridges 1") != std::string::npos) newConfigFile << "";
			if (line.find("ClientTransportPlugin snowflake") != std::string::npos) newConfigFile << "";
			if (line.find("Bridge snowflake") != std::string::npos) newConfigFile << "";
			newConfigFile << line << "\n";
		}

		newConfigFile.close();
		configFile.close();
		remove(m_torrcPath);
		rename("temp.torrc", m_torrcPath);

		return;
	}
};