#pragma once

#include <Core/Enums/Environment.h>

#include <string>
#include <filesystem.h>
#include <json/json.h>

class Config
{
public:
	using Ptr = std::shared_ptr<Config>;

	static Config::Ptr Load(const Environment environment);
	static fs::path DefaultDataDir(const Environment environment);
	static std::shared_ptr<Config> Load(const Json::Value& json, const Environment environment);
	static std::shared_ptr<Config> Default(const Environment environment);

	Json::Value& GetJSON() noexcept;

	const std::string& GetLogLevel() const noexcept;
	const fs::path& GetDataDirectory() const noexcept;
	const fs::path& GetLogDirectory() const noexcept;

	//
	// Node
	//
	const fs::path& GetChainPath() const noexcept;
	const fs::path& GetDatabasePath() const noexcept;
	const fs::path& GetTxHashSetPath() const noexcept;
	uint16_t GetRestAPIPort() const noexcept;
	uint64_t GetFeeBase() const noexcept;

	//
	// P2P
	//
	int GetMinPeers() const noexcept;
	void SetMinPeers(int min_peers) noexcept;

	int GetMaxPeers() const noexcept;
	void SetMaxPeers(int max_peers) noexcept;

	uint16_t GetP2PPort() const noexcept;
	const std::vector<uint8_t>& GetMagicBytes() const noexcept;

	uint8_t GetMinSyncPeers() const noexcept;

	//
	// Dandelion
	//
	uint16_t GetRelaySeconds() const noexcept;
	uint16_t GetEmbargoSeconds() const noexcept;
	uint8_t GetPatienceSeconds() const noexcept;
	uint8_t GetStemProbability() const noexcept;

	//
	// Wallet
	//
	const fs::path& GetWalletPath() const noexcept;
	uint32_t GetOwnerPort() const noexcept;
	uint32_t GetPublicKeyVersion() const noexcept;
	uint32_t GetPrivateKeyVersion() const noexcept;

	uint32_t GetMinimumConfirmations() const noexcept;
	void SetMinConfirmations(uint32_t min_confirmations) noexcept;

	bool ShouldReuseAddresses() const noexcept;
	void ShouldReuseAddresses(bool reuse_addresses) noexcept;
	//
	// TOR
	//
	const fs::path& GetTorDataPath() const noexcept;
	uint16_t GetSocksPort() const noexcept;
	uint16_t GetControlPort() const noexcept;
	const std::string& GetControlPassword() const noexcept;
	const std::string& GetHashedControlPassword() const noexcept;

private:
	Config(const Json::Value& json, const Environment environment, const fs::path& dataPath);
	
	struct Impl;
	std::shared_ptr<Impl> m_pImpl;
};

typedef std::shared_ptr<Config> ConfigPtr;