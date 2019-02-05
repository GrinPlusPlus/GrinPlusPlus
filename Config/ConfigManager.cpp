#include <Config/ConfigManager.h>

#include "ConfigReader.h"
#include "ConfigWriter.h"

#include <json/json.h>
#include <Infrastructure/Logger.h>
#include <StringUtil.h>
#include <filesystem>
#include <fstream>

Config ConfigManager::LoadConfig()
{
	const std::string currentDir = std::filesystem::current_path().string();
	const std::string configPath = currentDir + "/mainnet.json"; // TODO: Check for --floonet flag.

	std::ifstream file(configPath, std::ifstream::binary);
	if (file.is_open())
	{
		Json::Value root;
		const bool jsonParsed = Json::Reader().parse(file, root, false);
		file.close();

		if (!jsonParsed)
		{
			LoggerAPI::LogWarning("ConfigManager::LoadConfig - Failed to parse config.json");
		}

		return ConfigReader().ReadConfig(root);
	}
	else
	{
		LoggerAPI::LogWarning(StringUtil::Format("ConfigManager::LoadConfig - config.json not found in %s. Creating config.json with defaults.", currentDir));

		Json::Value emptyRoot;
		const Config defaultConfig = ConfigReader().ReadConfig(emptyRoot);

		ConfigWriter().SaveConfig(defaultConfig, configPath);

		return defaultConfig;
	}
}