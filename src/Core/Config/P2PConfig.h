#pragma once

#include "ConfigProps.h"

#include <cstdint>
#include <json/json.h>
#include <Core/Enums/Environment.h>

class P2PConfig
{
public:
	int GetMaxConnections() const noexcept { return m_maxConnections; }
	void SetMaxConnections(const int max_connections) noexcept { m_maxConnections = max_connections; }

	int GetMinConnections() const noexcept { return m_minConnections; }
	void SetMinConnections(const int min_connections) noexcept { m_minConnections = min_connections; }

	uint16_t GetP2PPort() const noexcept { return m_port; }
	const std::vector<uint8_t>& GetMagicBytes() const noexcept { return m_magicBytes; }

	//
	// Constructor
	//
	P2PConfig(const Environment env, const Json::Value& json)
	{
		if (env == Environment::MAINNET) {
			m_port = 3414;
			m_magicBytes = { 97, 61 };
		} else {
			m_port = 13414;
			m_magicBytes = { 83, 59 };
		}

		m_maxConnections = 50;
		m_minConnections = 15;

		if (json.isMember(ConfigProps::P2P::P2P))
		{
			const Json::Value& p2pJSON = json[ConfigProps::P2P::P2P];

			if (p2pJSON.isMember(ConfigProps::P2P::MAX_PEERS))
			{
				m_maxConnections = p2pJSON.get(ConfigProps::P2P::MAX_PEERS, 50).asInt();
			}

			if (p2pJSON.isMember(ConfigProps::P2P::MIN_PEERS))
			{
				m_minConnections = p2pJSON.get(ConfigProps::P2P::MIN_PEERS, 15).asInt();
			}
		}
	}

private:
	int m_maxConnections;
	int m_minConnections;
	uint16_t m_port;
	std::vector<uint8_t> m_magicBytes;
};