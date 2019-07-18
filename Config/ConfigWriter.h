#pragma once

#include <Config/Config.h>
#include <json/json.h>
#include <string>

class ConfigWriter
{
public:
	bool SaveConfig(const Config& config, const std::string& configPath) const;
	
private:
	void WriteClientMode(Json::Value& root, const EClientMode clientMode) const;
	void WriteDataPath(Json::Value& root, const std::string& dataPath) const;
	void WriteP2P(Json::Value& root, const P2PConfig& p2pConfig) const;
	void WriteDandelion(Json::Value& root, const DandelionConfig& dandelionConfig) const;
	void WriteServer(Json::Value& root, const ServerConfig& serverConfig) const;
	void WriteLogLevel(Json::Value& root, const std::string& logLevel) const;
	void WriteWalletConfig(Json::Value& root, const std::string& databaseType) const;
};