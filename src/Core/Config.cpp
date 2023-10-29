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
#include <Net/IPAddress.h>

#include <fstream>
#include <unordered_set>

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

Config::Ptr Config::Load(const fs::path& configPath, const Environment environment)
{
	std::shared_ptr<Config> pConfig = nullptr;
	
	// Read config file
	try {
		std::vector<unsigned char> data;
		if (!FileUtil::ReadFile(configPath, data)) {
			LOG_ERROR_F("Failed to open config file at: {}", configPath);
			throw FILE_EXCEPTION_F("Failed to open config file at: {}", configPath);
		}
		pConfig = Config::Load(JsonUtil::Parse(data), environment);
	}
	catch (...) { }
	
	if (pConfig == nullptr || pConfig->GetJSON().empty()) {
		pConfig = Config::Default(environment);
	}

	FileUtil::CreateDirectories(pConfig->GetDataDirectory());

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
	
	return std::shared_ptr<Config>(new Config(json, environment, dataDir));
}

fs::path Config::DefaultDataDir(const Environment environment)
{
	return FileUtil::GetHomeDirectory() / ".GrinPP" / Env::ToString(environment);
}

std::shared_ptr<Config> Config::Default(const Environment environment)
{
	Json::Value m_json = Json::Value(); 
	m_json["P2P"] = Json::Value();
	m_json["P2P"]["MAX_PEERS"] = 60;
	m_json["P2P"]["MIN_PEERS"] = 10;
	m_json["WALLET"] = Json::Value();
	m_json["WALLET"]["DATABASE"] = "SQLITE";
	m_json["WALLET"]["MIN_CONFIRMATIONS"] = 10;

	return Load(m_json, environment);
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
uint16_t Config::GetNodeAPIPort() const noexcept { return m_pImpl->m_nodeConfig.GetNodeAPIPort(); }
uint16_t Config::GetOwnerAPIPort() const noexcept { return m_pImpl->m_nodeConfig.GetOwnerAPIPort(); }
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

IPAddress Config::GetP2PIP() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetLocalhostIP(); }

uint8_t Config::GetMinSyncPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetMinSyncPeers(); }

const std::unordered_set<IPAddress>& Config::GetPreferredPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetPreferredPeers(); }
const std::unordered_set<IPAddress>& Config::GetAllowedPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetAllowedPeers(); }
const std::unordered_set<IPAddress>& Config::GetBlockedPeers() const noexcept { return m_pImpl->m_nodeConfig.GetP2P().GetBlockedPeers(); }

bool Config::IsPeerAllowed(const IPAddress& peer) {
	const std::unordered_set<IPAddress>& vec = m_pImpl->m_nodeConfig.GetP2P().GetAllowedPeers();
	if(vec.size() > 0) return std::find(vec.begin(), vec.end(), peer) != vec.end();
	return true;
}

bool Config::IsPeerBlocked(const IPAddress& peer) {
	const std::unordered_set<IPAddress>& vec = m_pImpl->m_nodeConfig.GetP2P().GetBlockedPeers();
	if(vec.size() > 0) return std::find(vec.begin(), vec.end(), peer) != vec.end();
	return false;
}

void Config::UpdatePreferredPeers(const std::unordered_set<IPAddress>& peers) noexcept { m_pImpl->m_nodeConfig.GetP2P().SetPreferredPeers(peers); }
void Config::UpdateAllowedPeers(const std::unordered_set<IPAddress>& peers) noexcept { m_pImpl->m_nodeConfig.GetP2P().SetAllowedPeers(peers); }
void Config::UpdateBlockedPeers(const std::unordered_set<IPAddress>& peers) noexcept { m_pImpl->m_nodeConfig.GetP2P().SetBlockedPeers(peers); }

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
uint32_t Config::GetWalletOwnerPort() const noexcept { return m_pImpl->m_walletConfig.GetWalletOwnerPort(); }
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
const fs::path& Config::GetTorrcPath() const noexcept { return m_pImpl->m_torConfig.GetTorrcPath(); }
void Config::AddObfs4TorBridge(const std::string bridge) noexcept { return m_pImpl->m_torConfig.AddObfs4TorBridge(bridge); }
void Config::ClearTorrcFile() noexcept { return m_pImpl->m_torConfig.ClearTorrcFile(); }
const std::string Config::GetTorrcFileContent() const noexcept { return m_pImpl->m_torConfig.ReadTorrcFile(); }
const uint16_t Config::GetSocksPort() const noexcept { return m_pImpl->m_torConfig.GetSocksPort(); }
const uint16_t Config::GetControlPort() const noexcept { return m_pImpl->m_torConfig.GetControlPort(); }
const std::string& Config::GetControlPassword() const noexcept { return m_pImpl->m_torConfig.GetControlPassword(); }
const std::string& Config::GetHashedControlPassword() const noexcept { return m_pImpl->m_torConfig.GetHashedControlPassword(); }
bool Config::IsTorBridgesEnabled() noexcept { return m_pImpl->m_torConfig.IsTorBridgesEnabled(); }
bool Config::EnableSnowflake() noexcept { return m_pImpl->m_torConfig.EnableSnowflake(); }
bool Config::DisableSnowflake() noexcept { return m_pImpl->m_torConfig.DisableSnowflake(); }
bool Config::DisableObfsBridges() noexcept { return m_pImpl->m_torConfig.DisableObfsBridges(); }
bool Config::IsObfs4Enabled() noexcept { return m_pImpl->m_torConfig.IsObfs4ConfigPresent(); }
bool Config::IsSnowflakeEnabled() noexcept { return m_pImpl->m_torConfig.IsSnowflakeConfigPresent(); }
void Config::DisableTorBridges() noexcept { return m_pImpl->m_torConfig.DisableTorBridges(); }
std::vector<std::string> Config::GetTorBridgesList() noexcept { return m_pImpl->m_torConfig.ListTorBridges(); }
