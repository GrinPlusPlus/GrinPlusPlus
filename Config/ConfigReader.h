#pragma once

#include <Config/Config.h>
#include <json/json.h>

class ConfigReader
{
public:
	Config ReadConfig(const Json::Value& root) const;

private:
	EClientMode ReadClientMode(const Json::Value& root) const;
	Environment ReadEnvironment(const Json::Value& root) const;
	std::string ReadDataPath(const Json::Value& root) const;
	P2PConfig ReadP2P(const Json::Value& root) const;
	DandelionConfig ReadDandelion(const Json::Value& root) const;
};