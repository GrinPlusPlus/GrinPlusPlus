#include <Core/Config.h>

#include "Config/ConfigProps.h"
#include "Config/NodeConfig.h"
#include "Config/WalletConfig.h"
#include "Config/TorConfig.h"

#include <Common/Logger.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Core/Exceptions/FileException.h>
#include <Core/Util/JsonUtil.h>
#include <fstream>

struct Config::Impl
{
	Impl(const Json::Value& json, const Environment environment, const fs::path& dataPath)
		: m_json(json),
		m_dataPath(dataPath),
		m_nodeConfig(environment, json, dataPath),
		m_walletConfig(json, environment, dataPath),
		m_torConfig(json, dataPath / "TOR")
	{
		fs::create_directories(m_dataPath);

		m_logPath = m_dataPath / "LOGS";
		fs::create_directories(m_logPath);

		m_logLevel = "DEBUG";
		if (json.isMember(ConfigProps::Logger::LOGGER)) {
			const Json::Value& loggerJSON = json[ConfigProps::Logger::LOGGER];
			m_logLevel = loggerJSON.get(ConfigProps::Logger::LOG_LEVEL, "DEBUG").asString();
		}
	}

	Json::Value m_json;

	fs::path m_dataPath;
	fs::path m_logPath;

	std::string m_logLevel;
	NodeConfig m_nodeConfig;
	WalletConfig m_walletConfig;
	TorConfig m_torConfig;
};

Config::Config(const Json::Value& json, const Environment environment, const fs::path& dataPath)
	: m_pImpl(std::make_shared<Impl>(json, environment, dataPath))
{

}

Config::Ptr Config::Load(const Environment environment)
{
	fs::path dataDir = FileUtil::GetHomeDirectory() / ".GrinPP" / Env::ToString(environment);
	FileUtil::CreateDirectories(dataDir);

	// Read config
	std::shared_ptr<Config> pConfig = nullptr;
	fs::path configPath = dataDir / "server_config.json";

	try {
		if (FileUtil::Exists(configPath)) {
			std::vector<unsigned char> data;
			if (!FileUtil::ReadFile(configPath, data)) {
				LOG_ERROR_F("Failed to open config file at: {}", configPath);
				throw FILE_EXCEPTION_F("Failed to open config file at: {}", configPath);
			}

			pConfig = Config::Load(JsonUtil::Parse(data), environment);
		}
	}
	catch (...) {}
	
	if (pConfig == nullptr) {
		pConfig = Config::Default(environment);
	}

	// Update config file
	Json::Value& json = pConfig->GetJSON();
	std::ofstream file(configPath, std::ios::out | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		LOG_ERROR_F("Failed to save config file at: {}", configPath);
		throw FILE_EXCEPTION_F("Failed to save config file at: {}", configPath);
	}

	file << json;
	file.close();

	return pConfig;
}

std::shared_ptr<Config> Config::Load(const Json::Value& json, const Environment environment)
{
	fs::path dataDir = DefaultDataDir(environment);

	if (json.isMember(ConfigProps::DATA_PATH)) {
		dataDir = fs::path(StringUtil::ToWide(json.get(ConfigProps::DATA_PATH, "").asString()));
	}

	FileUtil::CreateDirectories(dataDir);

	return std::shared_ptr<Config>(new Config(json, environment, dataDir));
}

fs::path Config::DefaultDataDir(const Environment environment)
{
	return FileUtil::GetHomeDirectory() / ".GrinPP" / Env::ToString(environment);
}

std::shared_ptr<Config> Config::Default(const Environment environment)
{
	return Load(Json::Value(), environment);
}

Json::Value& Config::GetJSON() noexcept { return m_pImpl->m_json; }

const std::string& Config::GetLogLevel() const noexcept { return m_pImpl->m_logLevel; }
const fs::path& Config::GetDataDirectory() const noexcept { return m_pImpl->m_dataPath; }
const fs::path& Config::GetLogDirectory() const noexcept { return m_pImpl->m_logPath; }

//
// Node
//
const fs::path& Config::GetChainPath() const noexcept { return m_pImpl->m_nodeConfig.GetChainPath(); }
const fs::path& Config::GetDatabasePath() const noexcept { return m_pImpl->m_nodeConfig.GetDatabasePath(); }
const fs::path& Config::GetTxHashSetPath() const noexcept { return m_pImpl->m_nodeConfig.GetTxHashSetPath(); }
uint16_t Config::GetRestAPIPort() const noexcept { return m_pImpl->m_nodeConfig.GetRestAPIPort(); }
uint64_t Config::GetFeeBase() const noexcept { return m_pImpl->m_nodeConfig.GetFeeBase(); }

//
// P2P
//
int Config::GetMinPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetMinConnections(); }
void Config::SetMinPeers(const int min_peers) noexcept { m_pImpl->m_nodeConfig.GetP2P().SetMinConnections(min_peers); }

int Config::GetMaxPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetMaxConnections(); }
void Config::SetMaxPeers(const int max_peers) noexcept { m_pImpl->m_nodeConfig.GetP2P().SetMaxConnections(max_peers); }

uint16_t Config::GetP2PPort() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetP2PPort(); }
const std::vector<uint8_t>& Config::GetMagicBytes() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetMagicBytes(); }

uint8_t Config::GetMinSyncPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetMinSyncPeers(); }

//
// Dandelion
//
uint16_t Config::GetRelaySeconds() const noexcept { return m_pImpl->m_nodeConfig.GetDandelion().GetRelaySeconds(); }
uint16_t Config::GetEmbargoSeconds() const noexcept { return m_pImpl->m_nodeConfig.GetDandelion().GetEmbargoSeconds(); }
uint8_t Config::GetPatienceSeconds() const noexcept { return m_pImpl->m_nodeConfig.GetDandelion().GetPatienceSeconds(); }
uint8_t Config::GetStemProbability() const noexcept { return m_pImpl->m_nodeConfig.GetDandelion().GetStemProbability(); }

//
// Wallet
//
const fs::path& Config::GetWalletPath() const noexcept { return m_pImpl->m_walletConfig.GetWalletPath(); }
uint32_t Config::GetOwnerPort() const noexcept { return m_pImpl->m_walletConfig.GetOwnerPort(); }
uint32_t Config::GetPublicKeyVersion() const noexcept { return m_pImpl->m_walletConfig.GetPublicKeyVersion(); }
uint32_t Config::GetPrivateKeyVersion() const noexcept { return m_pImpl->m_walletConfig.GetPrivateKeyVersion(); }

uint32_t Config::GetMinimumConfirmations() const noexcept { return m_pImpl->m_walletConfig.GetMinimumConfirmations(); }
void Config::SetMinConfirmations(const uint32_t min_confirmations) noexcept { return m_pImpl->m_walletConfig.SetMinConfirmations(min_confirmations); }
bool Config::ShouldReuseAddresses() const noexcept { return m_pImpl->m_walletConfig.GetReuseAddress() == 0 ? false : true; }
void Config::ShouldReuseAddresses(const bool reuse_addresses) noexcept { return m_pImpl->m_walletConfig.SetReuseAddress(reuse_addresses ? 1 : 0); }

//
// TOR
//
const fs::path& Config::GetTorDataPath() const noexcept { return m_pImpl->m_torConfig.GetTorDataPath(); }
uint16_t Config::GetSocksPort() const noexcept { return m_pImpl->m_torConfig.GetSocksPort(); }
uint16_t Config::GetControlPort() const noexcept { return m_pImpl->m_torConfig.GetControlPort(); }
const std::string& Config::GetControlPassword() const noexcept { return m_pImpl->m_torConfig.GetControlPassword(); }
const std::string& Config::GetHashedControlPassword() const noexcept { return m_pImpl->m_torConfig.GetHashedControlPassword(); }