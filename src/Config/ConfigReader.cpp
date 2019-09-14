#include "ConfigReader.h"
#include "ConfigProps.h"

#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <Common/Util/BitUtil.h>
#include <Common/Util/FileUtil.h>

Config ConfigReader::ReadConfig(const Json::Value& root, const EEnvironmentType environmentType) const
{
	// Read Mode
	const EClientMode clientMode = ReadClientMode(root);

	// Read Environment
	const Environment environment = ReadEnvironment(root, environmentType);

	// Read Directories
	const std::string dataPath = ReadDataPath(root, environment.GetEnvironmentType());

	// Read P2P Config
	const P2PConfig p2pConfig = ReadP2P(root);

	// Read Dandelion Config
	const DandelionConfig dandelionConfig = ReadDandelion(root);

	// Read Wallet Config
	const WalletConfig walletConfig = ReadWalletConfig(root, environment.GetEnvironmentType(), dataPath);

	// Read Server Config
	const ServerConfig serverConfig = ReadServerConfig(root, environmentType);

	// Read LogLevel
	const std::string logLevel = ReadLogLevel(root);

	// TODO: Mempool, mining, and logger settings
	return Config(clientMode, environment, dataPath, dandelionConfig, p2pConfig, walletConfig, serverConfig, logLevel);
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

Environment ConfigReader::ReadEnvironment(const Json::Value& root, const EEnvironmentType environmentType) const
{
	const uint16_t MAINNET_PORT = 3414;
	const uint16_t FLOONET_PORT = 13414;

	const std::vector<unsigned char> MAINNET_MAGIC = { 97, 61 };
	const std::vector<unsigned char> FLOONET_MAGIC = { 83, 59 };

	if (environmentType == EEnvironmentType::MAINNET)
	{
		return Environment(EEnvironmentType::MAINNET, Genesis::MAINNET_GENESIS, MAINNET_MAGIC, MAINNET_PORT);
	}
	else                         
	{
		return Environment(EEnvironmentType::FLOONET, Genesis::FLOONET_GENESIS, FLOONET_MAGIC, FLOONET_PORT);
	}
}

std::string ConfigReader::ReadDataPath(const Json::Value& root, const EEnvironmentType environmentType) const
{
	const std::string dataDirectory = environmentType == EEnvironmentType::MAINNET ? "MAINNET" : "FLOONET";
	const std::string defaultPath = FileUtil::GetHomeDirectory() + "/.GrinPP/" + dataDirectory + "/"; //+ SEPARATOR + ".GrinPP" + SEPARATOR + dataDirectory  + SEPARATOR;
	if (root.isMember(ConfigProps::DATA_PATH))
	{
		return root.get(ConfigProps::DATA_PATH, defaultPath).asString();
	}

	return defaultPath;
}

P2PConfig ConfigReader::ReadP2P(const Json::Value& root) const
{
	int maxPeers = 50;
	int minPeers = 15;

	if (root.isMember(ConfigProps::P2P::P2P))
	{
		const Json::Value& p2pRoot = root[ConfigProps::P2P::P2P];

		if (p2pRoot.isMember(ConfigProps::P2P::MAX_PEERS))
		{
			maxPeers = p2pRoot.get(ConfigProps::P2P::MAX_PEERS, 50).asInt();
		}

		if (p2pRoot.isMember(ConfigProps::P2P::MIN_PEERS))
		{
			minPeers = p2pRoot.get(ConfigProps::P2P::MIN_PEERS, 15).asInt();
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

WalletConfig ConfigReader::ReadWalletConfig(const Json::Value& root, const EEnvironmentType environmentType, const std::string& dataPath) const
{
	const std::string walletPath = dataPath + "WALLET/";

	std::string databaseType = "ROCKSDB";
	if (root.isMember(ConfigProps::Wallet::WALLET))
	{
		const Json::Value& walletRoot = root[ConfigProps::Wallet::WALLET];
		if (walletRoot.isMember(ConfigProps::Wallet::DATABASE))
		{
			databaseType = walletRoot.get(ConfigProps::Wallet::DATABASE, "ROCKSDB").asString();
		}
	}

	uint32_t listenPort = 3415; // TODO: Read value
	uint32_t ownerPort = 3420; // TODO: Read value
	if (environmentType == EEnvironmentType::FLOONET)
	{
		listenPort = 13415;
		ownerPort = 13420;
	}

	uint32_t privateKeyVersion = BitUtil::ConvertToU32(0x03, 0x27, 0x3A, 0x10);
	uint32_t publicKeyVersion = BitUtil::ConvertToU32(0x03, 0x27, 0x3E, 0x4B);
	if (environmentType == EEnvironmentType::MAINNET)
	{
		privateKeyVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x04, 0xA4);
		publicKeyVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x08, 0xDF);
	}

	const uint32_t minimumConfirmations = 10; // TODO: Read value

	return WalletConfig(walletPath, databaseType, listenPort, ownerPort, publicKeyVersion, privateKeyVersion, minimumConfirmations);
}

ServerConfig ConfigReader::ReadServerConfig(const Json::Value& root, const EEnvironmentType environmentType) const
{
	uint32_t restAPIPort = 3413;
	if (environmentType == EEnvironmentType::FLOONET)
	{
		restAPIPort = 13413;
	}

	if (root.isMember(ConfigProps::Server::SERVER))
	{
		const Json::Value& serverRoot = root[ConfigProps::Server::SERVER];

		if (serverRoot.isMember(ConfigProps::Server::REST_API_PORT))
		{
			restAPIPort = serverRoot.get(ConfigProps::Server::REST_API_PORT, restAPIPort).asInt();
		}
	}

	return ServerConfig(restAPIPort);
}

std::string ConfigReader::ReadLogLevel(const Json::Value& root) const
{
	if (root.isMember(ConfigProps::Logger::LOGGER))
	{
		const Json::Value& loggerRoot = root[ConfigProps::Logger::LOGGER];

		if (loggerRoot.isMember(ConfigProps::Logger::LOG_LEVEL))
		{
			return loggerRoot.get(ConfigProps::Logger::LOG_LEVEL, "DEBUG").asString();
		}
	}

	return "DEBUG";
}