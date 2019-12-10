#pragma once

#include <Common/Util/FileUtil.h>
#include <Config/DandelionConfig.h>
#include <Config/ClientMode.h>
#include <Config/P2PConfig.h>

#include <cstdint>
#include <json/json.h>

class NodeConfig
{
public:
	//
	// Getters
	//
	const P2PConfig& GetP2P() const { return m_p2pConfig; }
	const DandelionConfig& GetDandelion() const { return m_dandelion; }
	EClientMode GetClientMode() const { return EClientMode::FAST_SYNC; }
	const fs::path& GetChainPath() const { return m_chainPath; }
	const fs::path& GetDatabasePath() const { return m_databasePath; }
	const fs::path& GetTxHashSetPath() const { return m_txHashSetPath; }

	//
	// Constructor
	//
	NodeConfig(const Json::Value& json, const fs::path& dataPath)
		: m_p2pConfig(json), m_dandelion(json)
	{
		const fs::path nodePath = FileUtil::ToPath(dataPath.u8string() + "NODE/");

		m_chainPath = FileUtil::ToPath(nodePath.u8string() + "CHAIN/");
		fs::create_directories(m_chainPath);

		m_databasePath = FileUtil::ToPath(nodePath.u8string() + "DB/");
		fs::create_directories(m_databasePath);

		m_txHashSetPath = FileUtil::ToPath(nodePath.u8string() + "TXHASHSET/");
		fs::create_directories(m_txHashSetPath);

		fs::create_directories(FileUtil::ToPath(m_txHashSetPath.u8string() + "kernel/"));
		fs::create_directories(FileUtil::ToPath(m_txHashSetPath.u8string() + "output/"));
		fs::create_directories(FileUtil::ToPath(m_txHashSetPath.u8string() + "rangeproof/"));
	}

private:
	fs::path m_chainPath;
	fs::path m_databasePath;
	fs::path m_txHashSetPath;

	P2PConfig m_p2pConfig;
	DandelionConfig m_dandelion;
};