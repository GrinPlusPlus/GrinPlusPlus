#pragma once

#include <Config/DandelionConfig.h>
#include <Config/ClientMode.h>
#include <Config/P2PConfig.h>
#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <Config/WalletConfig.h>
#include <Config/ServerConfig.h>
#include <Config/TorConfig.h>
#include <string>
#include <filesystem.h>
#include <Common/Util/FileUtil.h>

class Config
{
public:
	Config(
		const EClientMode clientMode, 
		const Environment& environment, 
		const fs::path& dataPath, 
		const DandelionConfig& dandelionConfig,
		const P2PConfig& p2pConfig,
		const WalletConfig& walletConfig, 
		const ServerConfig& serverConfig,
		const std::string& logLevel,
		const TorConfig& torConfig,
		const std::string& grinjoinSecretKey
	)
		: m_clientMode(clientMode), 
		m_environment(environment), 
		m_dataPath(dataPath), 
		m_dandelionConfig(dandelionConfig),
		m_p2pConfig(p2pConfig),
		m_walletConfig(walletConfig),
		m_serverConfig(serverConfig),
		m_logLevel(logLevel),
		m_torConfig(torConfig),
		m_grinjoinSecretKey(grinjoinSecretKey)
	{
		std::wstring nodePath = m_dataPath.wstring() + L"NODE/";
		m_nodePath = fs::path(nodePath);

		m_logPath = fs::path(nodePath + L"LOGS/");
		fs::create_directories(m_logPath);

		m_chainPath = fs::path(nodePath + L"CHAIN/");
		fs::create_directories(m_chainPath);

		m_databasePath = fs::path(nodePath + L"DB/");
		fs::create_directories(m_databasePath);

		m_txHashSetPath = fs::path(nodePath + L"TXHASHSET/");
		fs::create_directories(m_txHashSetPath);

		fs::create_directories(m_txHashSetPath.wstring() + L"kernel/");
		fs::create_directories(m_txHashSetPath.wstring() + L"output/");
		fs::create_directories(m_txHashSetPath.wstring() + L"rangeproof/");
	}

	const fs::path& GetDataDirectory() const { return m_dataPath; }
	fs::path GetNodeDirectory() const { return m_nodePath; }
	fs::path GetLogDirectory() const { return m_logPath; }
	fs::path GetChainDirectory() const { return m_chainPath; }
	fs::path GetDatabaseDirectory() const { return m_databasePath; }
	fs::path GetTxHashSetDirectory() const { return m_txHashSetPath; }

	const Environment& GetEnvironment() const { return m_environment; }
	const DandelionConfig& GetDandelionConfig() const { return m_dandelionConfig; }
	const P2PConfig& GetP2PConfig() const { return m_p2pConfig; }
	const EClientMode GetClientMode() const { return EClientMode::FAST_SYNC; }
	const WalletConfig& GetWalletConfig() const { return m_walletConfig; }
	const ServerConfig& GetServerConfig() const { return m_serverConfig; }
	const std::string& GetLogLevel() const { return m_logLevel; }
	const TorConfig& GetTorConfig() const { return m_torConfig; }
	const std::string& GetGrinJoinSecretKey() const { return m_grinjoinSecretKey; }

private:
	fs::path m_dataPath;
	fs::path m_nodePath;
	fs::path m_logPath;
	fs::path m_chainPath;
	fs::path m_databasePath;
	fs::path m_txHashSetPath;

	EClientMode m_clientMode;
	
	DandelionConfig m_dandelionConfig;
	P2PConfig m_p2pConfig;
	Environment m_environment;
	WalletConfig m_walletConfig;
	ServerConfig m_serverConfig;
	std::string m_logLevel;
	TorConfig m_torConfig;
	std::string m_grinjoinSecretKey;
};

typedef std::shared_ptr<const Config> ConfigPtr;