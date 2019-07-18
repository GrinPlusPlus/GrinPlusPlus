#include <Config/ConfigManager.h>

#include "ConfigReader.h"
#include "ConfigWriter.h"

#include <json/json.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Core/Util/JsonUtil.h>
#include <fstream>

Config ConfigManager::LoadConfig(const EEnvironmentType environment)
{
	const std::string dataDirectory = (environment == EEnvironmentType::MAINNET ? "MAINNET" : "FLOONET");
	const std::string configPath = FileUtil::GetHomeDirectory() + "/.GrinPP/" + dataDirectory + "/";
	FileUtil::CreateDirectories(configPath);
	const std::string configFile = configPath + "server_config.json";

	std::ifstream file(configFile, std::ifstream::binary);
	if (file.is_open())
	{
		Json::Value root;
		std::string errors;
		const bool jsonParsed = Json::parseFromStream(Json::CharReaderBuilder(), file, &root, &errors);
		file.close();

		if (!jsonParsed)
		{
			LoggerAPI::LogWarning("ConfigManager::LoadConfig - Failed to parse config.json");
		}

		const Config config = ConfigReader().ReadConfig(root, environment);

		ConfigWriter().SaveConfig(config, configFile);

		return config;
	}
	else
	{
		LoggerAPI::LogWarning(StringUtil::Format("ConfigManager::LoadConfig - server_config.json not found in %s. Creating server_config.json with defaults.", configPath.c_str()));

		Json::Value emptyRoot;
		const Config defaultConfig = ConfigReader().ReadConfig(emptyRoot, environment);

		ConfigWriter().SaveConfig(defaultConfig, configFile);

		return defaultConfig;
	}
}