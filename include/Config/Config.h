#pragma once

#include <Config/DandelionConfig.h>
#include <Config/ClientMode.h>
#include <Config/P2PConfig.h>
#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <Config/WalletConfig.h>
#include <Config/ServerConfig.h>
#include <string>
#include <filesystem>

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
		const std::string& logLevel
	)
		: m_clientMode(clientMode), 
		m_environment(environment), 
		m_dataPath(dataPath), 
		m_dandelionConfig(dandelionConfig),
		m_p2pConfig(p2pConfig),
		m_walletConfig(walletConfig),
		m_serverConfig(serverConfig),
		m_logLevel(logLevel)
	{
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_txHashSetPath);
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_txHashSetPath + "kernel/");
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_txHashSetPath + "output/");
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_txHashSetPath + "rangeproof/");
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_chainPath);
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_databasePath);
		std::filesystem::create_directories(m_dataPath + "NODE\\" + m_logDirectory);
	}

	inline const std::string& GetDataDirectory() const { return m_dataPath; }
	inline const std::string GetLogDirectory() const { return m_dataPath + "NODE\\" + m_logDirectory; }
	inline const std::string GetChainDirectory() const { return m_dataPath + "NODE\\" + m_chainPath; }
	inline const std::string GetDatabaseDirectory() const { return m_dataPath + "NODE\\" + m_databasePath; }
	inline const std::string GetTxHashSetDirectory() const { return m_dataPath + "NODE\\" + m_txHashSetPath; }

	inline const Environment& GetEnvironment() const { return m_environment; }
	inline const DandelionConfig& GetDandelionConfig() const { return m_dandelionConfig; }
	inline const P2PConfig& GetP2PConfig() const { return m_p2pConfig; }
	inline const EClientMode GetClientMode() const { return EClientMode::FAST_SYNC; }
	inline const WalletConfig& GetWalletConfig() const { return m_walletConfig; }
	inline const ServerConfig& GetServerConfig() const { return m_serverConfig; }
	inline const std::string& GetLogLevel() const { return m_logLevel; }

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
};