#pragma once

#include <cstdint>
#include <json/json.h>
#include <Config/ConfigProps.h>
#include <Config/EnvironmentType.h>

class ServerConfig
{
public:
	//
	// Getters
	//
	uint32_t GetRestAPIPort() const { return m_restAPIPort; }
	const std::string& GetGrinJoinSecretKey() const { return m_grinjoinSecretKey; }

	//
	// Constructor
	//
	ServerConfig(const Json::Value& json, const EEnvironmentType environment)
	{
		if (environment == EEnvironmentType::MAINNET)
		{
			m_restAPIPort = 3413;
		}
		else
		{
			m_restAPIPort = 13413;
		}

		if (json.isMember(ConfigProps::Server::SERVER))
		{
			const Json::Value& serverJSON = json[ConfigProps::Server::SERVER];

			if (serverJSON.isMember(ConfigProps::Server::REST_API_PORT))
			{
				m_restAPIPort = serverJSON.get(ConfigProps::Server::REST_API_PORT, m_restAPIPort).asInt();
			}
		}
	}

private:
	uint32_t m_restAPIPort;
	std::string m_grinjoinSecretKey;
};