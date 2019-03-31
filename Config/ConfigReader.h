#pragma once

#include <Config/Config.h>
#include <json/json.h>

class ConfigReader
{
public:
	Config ReadConfig(const Json::Value& root, const EEnvironmentType environmentType) const;

private:
	EClientMode ReadClientMode(const Json::Value& root) const;
	Environment ReadEnvironment(const Json::Value& root, const EEnvironmentType environmentType) const;
	std::string ReadDataPath(const Json::Value& root, const EEnvironmentType environmentType) const;
	P2PConfig ReadP2P(const Json::Value& root) const;
	DandelionConfig ReadDandelion(const Json::Value& root) const;
	WalletConfig ReadWalletConfig(const Json::Value& root, const EEnvironmentType environmentType, const std::string& dataPath) const;
	ServerConfig ReadServerConfig(const Json::Value& root, const EEnvironmentType environmentType) const;
	std::string ReadLogLevel(const Json::Value& root) const;
};