#pragma once

#include <Config/DandelionConfig.h>
#include <Config/ClientMode.h>
#include <Config/P2PConfig.h>
#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <Config/WalletConfig.h>
#include <string>
#include <filesystem>

class Config
{
public:
	Config(const EClientMode clientMode, const Environment& environment, const std::string& dataPath, const DandelionConfig& dandelionConfig, const P2PConfig& p2pConfig, const WalletConfig& walletConfig)
		: m_clientMode(clientMode), m_environment(environment), m_dataPath(dataPath), m_dandelionConfig(dandelionConfig), m_p2pConfig(p2pConfig), m_walletConfig(walletConfig)
	{
		std::filesystem::create_directories(m_dataPath + m_txHashSetPath);
		std::filesystem::create_directories(m_dataPath + m_txHashSetPath + "kernel/");
		std::filesystem::create_directories(m_dataPath + m_txHashSetPath + "output/");
		std::filesystem::create_directories(m_dataPath + m_txHashSetPath + "rangeproof/");
		std::filesystem::create_directories(m_dataPath + m_chainPath);
		std::filesystem::create_directories(m_dataPath + m_databasePath);
		std::filesystem::create_directories(m_dataPath + m_logDirectory);
	}

	inline const std::string& GetDataDirectory() const { return m_dataPath; }
	inline const std::string GetLogDirectory() const { return m_dataPath + m_logDirectory; }
	inline const std::string GetChainDirectory() const { return m_dataPath + m_chainPath; }
	inline const std::string GetDatabaseDirectory() const { return m_dataPath + m_databasePath; }
	inline const std::string GetTxHashSetDirectory() const { return m_dataPath + m_txHashSetPath; }

	inline const Environment& GetEnvironment() const { return m_environment; }
	inline const DandelionConfig& GetDandelionConfig() const { return m_dandelionConfig; }
	inline const P2PConfig& GetP2PConfig() const { return m_p2pConfig; }
	inline const EClientMode GetClientMode() const { return EClientMode::FAST_SYNC; }
	inline const WalletConfig& GetWalletConfig() const { return m_walletConfig; }

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
};