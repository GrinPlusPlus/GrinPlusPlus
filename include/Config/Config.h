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
		const std::string& dataPath, 
		const DandelionConfig& dandelionConfig,
		const P2PConfig& p2pConfig,
		const WalletConfig& walletConfig, 
		const ServerConfig& serverConfig,
		const std::string& logLevel,
		const TorConfig& torConfig
	)
		: m_clientMode(clientMode), 
		m_environment(environment), 
		m_dataPath(dataPath), 
		m_dandelionConfig(dandelionConfig),
		m_p2pConfig(p2pConfig),
		m_walletConfig(walletConfig),
		m_serverConfig(serverConfig),
		m_logLevel(logLevel),
		m_torConfig(torConfig)
	{
		fs::create_directories(m_dataPath + "NODE/" + m_txHashSetPath);
		fs::create_directories(m_dataPath + "NODE/" + m_txHashSetPath + "kernel/");
		fs::create_directories(m_dataPath + "NODE/" + m_txHashSetPath + "output/");
		fs::create_directories(m_dataPath + "NODE/" + m_txHashSetPath + "rangeproof/");
		fs::create_directories(m_dataPath + "NODE/" + m_chainPath);
		fs::create_directories(m_dataPath + "NODE/" + m_databasePath);
		fs::create_directories(m_dataPath + "NODE/" + m_logDirectory);
	}

	const std::string& GetDataDirectory() const { return m_dataPath; }
	std::string GetNodeDirectory() const { return m_dataPath + "NODE/"; }
	std::string GetLogDirectory() const { return m_dataPath + "NODE/" + m_logDirectory; }
	std::string GetChainDirectory() const { return m_dataPath + "NODE/" + m_chainPath; }
	std::string GetDatabaseDirectory() const { return m_dataPath + "NODE/" + m_databasePath; }
	std::string GetTxHashSetDirectory() const { return m_dataPath + "NODE/" + m_txHashSetPath; }

	const Environment& GetEnvironment() const { return m_environment; }
	const DandelionConfig& GetDandelionConfig() const { return m_dandelionConfig; }
	const P2PConfig& GetP2PConfig() const { return m_p2pConfig; }
	const EClientMode GetClientMode() const { return EClientMode::FAST_SYNC; }
	const WalletConfig& GetWalletConfig() const { return m_walletConfig; }
	const ServerConfig& GetServerConfig() const { return m_serverConfig; }
	const std::string& GetLogLevel() const { return m_logLevel; }
	const TorConfig& GetTorConfig() const { return m_torConfig; }

private:
	std::string m_txHashSetPath{ "TXHASHSET/" };
	std::string m_chainPath{ "CHAIN/" };
	std::string m_logDirectory{ "LOGS/" };
	std::string m_databasePath{ "DB/" };
	std::string m_dataPath;
	EClientMode m_clientMode;
	
	DandelionConfig m_dandelionConfig;
	P2PConfig m_p2pConfig;
	Environment m_environment;
	WalletConfig m_walletConfig;
	ServerConfig m_serverConfig;
	std::string m_logLevel;
	TorConfig m_torConfig;
};

typedef std::shared_ptr<const Config> ConfigPtr;