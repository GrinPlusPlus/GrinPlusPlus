#pragma once

#include <Config/Config.h>
#include <Config/ConfigProps.h>
#include <Common/Util/FileUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Exceptions/FileException.h>
#include <Infrastructure/Logger.h>
#include <fstream>

class ConfigLoader
{
public:
	static ConfigPtr Load(const EEnvironmentType environment)
	{
		std::shared_ptr<Config> pConfig = nullptr;

		fs::path dataDir = FileUtil::GetHomeDirectory() / ".GrinPP" / Env::ToString(environment);
		FileUtil::CreateDirectories(dataDir);

		fs::path configPath = dataDir / "server_config.json";
		if (FileUtil::Exists(configPath))
		{
			std::vector<unsigned char> data;
			if (!FileUtil::ReadFile(configPath, data))
			{
				LOG_ERROR_F("Failed to open config file at: {}", configPath);
				throw FILE_EXCEPTION_F("Failed to open config file at: {}", configPath);
			}

			pConfig = Config::Load(JsonUtil::Parse(data), environment);
		}
		else
		{
			pConfig = Config::Default(environment);
		}

		UpdateConfig(configPath, pConfig);
		return pConfig;
	}

private:
	static void UpdateConfig(const fs::path& path, std::shared_ptr<Config> pConfig)
	{
		Json::Value& json = pConfig->GetJSON();

		if (!json.isMember(ConfigProps::Wallet::WALLET))
		{
			json[ConfigProps::Wallet::WALLET] = Json::Value();
		}

		Json::Value& walletRoot = json[ConfigProps::Wallet::WALLET];
		if (!walletRoot.isMember(ConfigProps::Wallet::DATABASE))
		{
			walletRoot[ConfigProps::Wallet::DATABASE] = "SQLITE";
		}

		std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			LOG_ERROR_F("Failed to save config file at: {}", path);
			throw FILE_EXCEPTION_F("Failed to save config file at: {}", path);
		}

		file << json;
		file.close();
	}
};