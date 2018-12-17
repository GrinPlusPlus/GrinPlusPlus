#include "ConfigReader.h"
#include "ConfigProps.h"

#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <filesystem>

Config ConfigReader::ReadConfig(const Json::Value& root) const
{
	// Read Mode
	const EClientMode clientMode = ReadClientMode(root);

	// Read Environment
	const Environment environment = ReadEnvironment(root);

	// Read Directories
	const std::string dataPath = ReadDataPath(root);

	// Read P2P Config
	const P2PConfig p2pConfig = ReadP2P(root);

	// Read Dandelion Config
	const DandelionConfig dandelionConfig = ReadDandelion(root);

	// TODO: Mempool, mining, wallet, and logger settings

	return Config(clientMode, environment, dataPath, dandelionConfig, p2pConfig);
}

EClientMode ConfigReader::ReadClientMode(const Json::Value& root) const
{
	if (root.isMember(ConfigProps::CLIENT_MODE))
	{
		const std::string mode = root.get(ConfigProps::CLIENT_MODE, "FAST_SYNC").asString();
		if (mode == "FAST_SYNC")
		{
			return EClientMode::FAST_SYNC;
		}
		else if (mode == "LIGHT_CLIENT")
		{
			return EClientMode::LIGHT_CLIENT;
		}
		else if (mode == "FULL_HISTORY")
		{
			return EClientMode::FULL_HISTORY;
		}
	}

	return EClientMode::FAST_SYNC;
}

Environment ConfigReader::ReadEnvironment(const Json::Value& root) const
{
	if (root.isMember(ConfigProps::ENVIRONMENT))
	{
		const std::string environment = root.get(ConfigProps::ENVIRONMENT, "TESTNET4").asString();
		if (environment == "MAINNET")
		{
			// MAINNET: Return mainnet Environment. Also check for permanent testnet.
		}
		else if (environment == "TESTNET4")
		{
			return Environment(Genesis::TESTNET4_GENESIS);
		}
	}


	return Environment(Genesis::TESTNET4_GENESIS);
}

std::string ConfigReader::ReadDataPath(const Json::Value& root) const
{
	const std::string defaultPath = std::filesystem::current_path().string() + "/DATA/";
	if (root.isMember(ConfigProps::DATA_PATH))
	{
		return root.get(ConfigProps::DATA_PATH, defaultPath).asString();
	}

	return defaultPath;
}

P2PConfig ConfigReader::ReadP2P(const Json::Value& root) const
{
	int maxPeers = 15;
	int minPeers = 8;

	if (root.isMember(ConfigProps::P2P::P2P))
	{
		const Json::Value& p2pRoot = root[ConfigProps::P2P::P2P];

		if (p2pRoot.isMember(ConfigProps::P2P::MAX_PEERS))
		{
			maxPeers = p2pRoot.get(ConfigProps::P2P::MAX_PEERS, 15).asInt();
		}

		if (p2pRoot.isMember(ConfigProps::P2P::MIN_PEERS))
		{
			minPeers = p2pRoot.get(ConfigProps::P2P::MIN_PEERS, 8).asInt();
		}
	}

	return P2PConfig(maxPeers, minPeers);
}

DandelionConfig ConfigReader::ReadDandelion(const Json::Value& root) const
{
	uint16_t relaySeconds = 600;
	uint16_t embargoSeconds = 180;
	uint8_t patienceSeconds = 10;
	uint8_t stemProbability = 90;

	if (root.isMember(ConfigProps::Dandelion::DANDELION))
	{
		const Json::Value& dandelionRoot = root[ConfigProps::Dandelion::DANDELION];

		if (dandelionRoot.isMember(ConfigProps::Dandelion::RELAY_SECS))
		{
			relaySeconds = (uint16_t)dandelionRoot.get(ConfigProps::Dandelion::RELAY_SECS, 600).asUInt();
		}

		if (dandelionRoot.isMember(ConfigProps::Dandelion::EMBARGO_SECS))
		{
			embargoSeconds = (uint16_t)dandelionRoot.get(ConfigProps::Dandelion::EMBARGO_SECS, 180).asUInt();
		}

		if (dandelionRoot.isMember(ConfigProps::Dandelion::PATIENCE_SECS))
		{
			patienceSeconds = (uint8_t)dandelionRoot.get(ConfigProps::Dandelion::PATIENCE_SECS, 10).asUInt();
		}

		if (dandelionRoot.isMember(ConfigProps::Dandelion::STEM_PROBABILITY))
		{
			stemProbability = (uint8_t)dandelionRoot.get(ConfigProps::Dandelion::STEM_PROBABILITY, 90).asUInt();
		}
	}

	return DandelionConfig(relaySeconds, embargoSeconds, patienceSeconds, stemProbability);
}