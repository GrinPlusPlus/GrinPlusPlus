#pragma once

#include <Config/ConfigProps.h>
#include <Config/NodeConfig.h>
#include <Config/WalletConfig.h>
#include <Config/ServerConfig.h>

#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <Config/TorConfig.h>
#include <string>
#include <filesystem.h>
#include <Common/Util/FileUtil.h>

class Config
{
public:
	using CPtr = std::shared_ptr<const Config>;

	static std::shared_ptr<Config> Load(const Json::Value& json, const EEnvironmentType environment)
	{
		fs::path dataDir = FileUtil::GetHomeDirectory() / ".GrinPP" / Env::ToString(environment);

		if (json.isMember(ConfigProps::DATA_PATH))
		{
			dataDir = fs::path(StringUtil::ToWide(json.get(ConfigProps::DATA_PATH, "").asString()));
		}

		FileUtil::CreateDirectories(dataDir);
		
		return std::shared_ptr<Config>(new Config(json, environment, dataDir));
	}

	static std::shared_ptr<Config> Default(const EEnvironmentType environment)
	{
		return Load(Json::Value(), environment);
	}

	Json::Value& GetJSON() noexcept { return m_json; }

	const std::string& GetLogLevel() const noexcept { return m_logLevel; }
	const Environment& GetEnvironment() const noexcept { return m_environment; }
	const fs::path& GetDataDirectory() const noexcept { return m_dataPath; }
	const fs::path& GetLogDirectory() const noexcept { return m_logPath; }
	const NodeConfig& GetNodeConfig() const noexcept { return m_nodeConfig; }
	const P2PConfig& GetP2PConfig() const noexcept { return m_nodeConfig.GetP2P(); }

	const WalletConfig& GetWalletConfig() const noexcept { return m_walletConfig; }
	const ServerConfig& GetServerConfig() const noexcept { return m_serverConfig; }
	const TorConfig& GetTorConfig() const noexcept { return m_torConfig; }

	uint16_t GetP2PPort() const noexcept { return m_environment.GetP2PPort(); }
	uint64_t GetFeeBase() const noexcept { return m_nodeConfig.GetFeeBase(); }

private:
	Config(const Json::Value& json, const EEnvironmentType environment, const fs::path& dataPath)
		: m_json(json),
		m_environment(environment),
		m_dataPath(dataPath),
		m_nodeConfig(m_json, dataPath),
		m_walletConfig(m_json, environment, m_dataPath),
		m_serverConfig(m_json, environment),
		m_torConfig(json, dataPath / "TOR")
	{
		fs::create_directories(m_dataPath);

		m_logPath = m_dataPath / "LOGS";
		fs::create_directories(m_logPath);

		m_logLevel = "DEBUG";
		if (json.isMember(ConfigProps::Logger::LOGGER))
		{
			const Json::Value& loggerJSON = json[ConfigProps::Logger::LOGGER];
			m_logLevel = loggerJSON.get(ConfigProps::Logger::LOG_LEVEL, "DEBUG").asString();
		}
	}

	Json::Value m_json;

	fs::path m_dataPath;
	fs::path m_logPath;

	std::string m_logLevel;
	Environment m_environment;
	NodeConfig m_nodeConfig;
	WalletConfig m_walletConfig;
	ServerConfig m_serverConfig;
	TorConfig m_torConfig;
};

typedef std::shared_ptr<const Config> ConfigPtr;