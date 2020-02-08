#pragma once

#include <cstdint>
#include <json/json.h>
#include <Config/ConfigProps.h>

class P2PConfig
{
public:
	//
	// Getters
	//
	int GetMaxConnections() const { return m_maxConnections; }
	int GetMinConnections() const { return m_minConnections; }

	//
	// Constructor
	//
	P2PConfig(const Json::Value& json)
	{
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
};