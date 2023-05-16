#pragma once

#include "ConfigProps.h"
#include "DandelionConfig.h"
#include "P2PConfig.h"

#include <Common/Util/FileUtil.h>
#include <cstdint>
#include <json/json.h>

class NodeConfig
{
public:
	//
	// Getters
	//
	P2PConfig& GetP2P() { return m_p2pConfig; }
	const DandelionConfig& GetDandelion() const { return m_dandelion; }
	const fs::path& GetChainPath() const { return m_chainPath; }
	const fs::path& GetDatabasePath() const { return m_databasePath; }
	const fs::path& GetTxHashSetPath() const { return m_txHashSetPath; }
	uint64_t GetFeeBase() const noexcept { return 500000; } // TODO: Read from config.
	uint16_t GetOwnerAPIPort() const { return m_ownerAPIPort; }

	//
	// Constructor
	//
	NodeConfig(const Environment env, const Json::Value& json, const fs::path& dataPath)
		: m_p2pConfig(env, json), m_dandelion(json)
	{
		if (env == Environment::MAINNET) {
			m_ownerAPIPort = 3420;
		} else {
			m_ownerAPIPort = 13420;
		}

		if (json.isMember(ConfigProps::Server::SERVER)) {
			const Json::Value& serverJSON = json[ConfigProps::Server::SERVER];

			if (serverJSON.isMember(ConfigProps::Server::REST_API_PORT)) {
				m_ownerAPIPort = (uint16_t)serverJSON.get(ConfigProps::Server::REST_API_PORT, m_ownerAPIPort).asInt();
			}
		}

		const fs::path nodePath = dataPath / "NODE";

		m_chainPath = nodePath / "CHAIN";
		fs::create_directories(m_chainPath);

		m_databasePath = nodePath / "DB";
		fs::create_directories(m_databasePath);

		m_txHashSetPath = nodePath / "TXHASHSET";
		fs::create_directories(m_txHashSetPath);

		fs::create_directories(m_txHashSetPath / "kernel");
		fs::create_directories(m_txHashSetPath / "output");
		fs::create_directories(m_txHashSetPath / "rangeproof");
	}

private:
	fs::path m_chainPath;
	fs::path m_databasePath;
	fs::path m_txHashSetPath;

	uint16_t m_ownerAPIPort;
	P2PConfig m_p2pConfig;
	DandelionConfig m_dandelion;
};