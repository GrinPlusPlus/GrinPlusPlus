#include "ConfigWriter.h"
#include "ConfigProps.h"

#include <Infrastructure/Logger.h>
#include <json/json.h>
#include <fstream>

bool ConfigWriter::SaveConfig(const Config& config, const std::string& configPath) const
{
	Json::Value root;
	WriteClientMode(root, config.GetClientMode());
	WriteDataPath(root, config.GetDataDirectory());
	WriteP2P(root, config.GetP2PConfig());
	WriteDandelion(root, config.GetDandelionConfig());
	WriteServer(root, config.GetServerConfig());
	WriteLogLevel(root, config.GetLogLevel());
	WriteWalletConfig(root, "SQLITE");

	std::ofstream file(configPath, std::ios::out | std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		LoggerAPI::LogError("Failed to save config file at " + configPath);
		return false;
	}

	file << root;
	file.close();

	return true;
}

void ConfigWriter::WriteClientMode(Json::Value& root, const EClientMode clientMode) const
{
	std::string mode = "FAST_SYNC";
	switch (clientMode)
	{
	case EClientMode::FAST_SYNC:
		mode = "FAST_SYNC";
		break;
	case EClientMode::LIGHT_CLIENT:
		mode = "LIGHT_CLIENT";
		break;
	case EClientMode::FULL_HISTORY:
		mode = "FULL_HISTORY";
		break;
	}

	Json::Value modeValue = Json::Value(mode);
	//const std::string modeComment = "/* Supported: FAST_SYNC, LIGHT_CLIENT, FULL_HISTORY */";
	//modeValue.setComment(modeComment, Json::commentBefore);
	root[ConfigProps::CLIENT_MODE] = modeValue;
}

void ConfigWriter::WriteDataPath(Json::Value& root, const std::string& dataPath) const
{
	Json::Value dataPathValue = Json::Value(dataPath);
	//const std::string dataPathComment = "/* The location where all chain, transaction, and peer data will be stored. Must be a full path. */";
	//dataPathValue.setComment(dataPathComment, Json::commentBefore);
	root[ConfigProps::DATA_PATH] = dataPathValue;
}

void ConfigWriter::WriteP2P(Json::Value& root, const P2PConfig& p2pConfig) const
{
	Json::Value p2pJSON;

	Json::Value maxPeersValue = Json::Value(p2pConfig.GetMaxConnections());
	//const std::string maxPeersComment = "/* The maximum number of peer connections. The node will refuse to accept new connections once the maximum is reached. */";
	//maxPeersValue.setComment(maxPeersComment, Json::commentBefore);
	p2pJSON[ConfigProps::P2P::MAX_PEERS] = maxPeersValue;

	Json::Value minPeersValue = Json::Value(p2pConfig.GetPreferredMinConnections());
	//const std::string minPeersComment = "/* The preferred minimum number of peer connections. The node will continuously try to connect to new peers until minimum is reached. */";
	//minPeersValue.setComment(minPeersComment, Json::commentBefore);
	p2pJSON[ConfigProps::P2P::MIN_PEERS] = minPeersValue;

	root[ConfigProps::P2P::P2P] = p2pJSON;
}

void ConfigWriter::WriteDandelion(Json::Value& root, const DandelionConfig& dandelionConfig) const
{
	Json::Value dandelionJSON;

	Json::Value relaySecondsValue = Json::Value(dandelionConfig.GetRelaySeconds());
	//const std::string relaySecondsComment = "/* Choose new Dandelion relay peer every n secs. */";
	//relaySecondsValue.setComment(relaySecondsComment, Json::commentBefore);
	dandelionJSON[ConfigProps::Dandelion::RELAY_SECS] = relaySecondsValue;

	Json::Value embargoSecondsValue = Json::Value(dandelionConfig.GetEmbargoSeconds());
	//const std::string embargoSecondsComment = "/* Dandelion embargo, fluff and broadcast tx if not seen on network before embargo expires. */";
	//embargoSecondsValue.setComment(embargoSecondsComment, Json::commentBefore);
	dandelionJSON[ConfigProps::Dandelion::EMBARGO_SECS] = embargoSecondsValue;

	Json::Value patienceSecondsValue = Json::Value(dandelionConfig.GetPatienceSeconds());
	//const std::string patienceSecondsComment = "/* Dandelion patience timer, fluff/stem processing runs every n secs. \n\t\tTx aggregation happens on stem txs received within this window. */";
	//patienceSecondsValue.setComment(patienceSecondsComment, Json::commentBefore);
	dandelionJSON[ConfigProps::Dandelion::PATIENCE_SECS] = patienceSecondsValue;

	Json::Value stemProbabilityValue = Json::Value(dandelionConfig.GetStemProbability());
	//const std::string stemProbabilityComment = "/* Dandelion stem probability (stem 90% of the time, fluff 10%). */";
	//stemProbabilityValue.setComment(stemProbabilityComment, Json::commentBefore);
	dandelionJSON[ConfigProps::Dandelion::STEM_PROBABILITY] = stemProbabilityValue;

	root[ConfigProps::Dandelion::DANDELION] = dandelionJSON;
}

void ConfigWriter::WriteServer(Json::Value& root, const ServerConfig& serverConfig) const
{
	Json::Value serverJSON;

	Json::Value restAPIPortValue = Json::Value(serverConfig.GetRestAPIPort());
	//const std::string restAPIPortComment = "/* Port number to host the node's Rest API. */";
	//restAPIPortValue.setComment(restAPIPortComment, Json::commentBefore);
	serverJSON[ConfigProps::Server::REST_API_PORT] = restAPIPortValue;

	root[ConfigProps::Server::SERVER] = serverJSON;
}

void ConfigWriter::WriteLogLevel(Json::Value& root, const std::string& logLevel) const
{
	Json::Value loggerJSON;

	Json::Value logLevelValue = Json::Value(logLevel);
	//const std::string logLevelComment = "/* Log level. */";
	//logLevelValue.setComment(logLevelComment, Json::commentBefore);
	loggerJSON[ConfigProps::Logger::LOG_LEVEL] = logLevelValue;

	root[ConfigProps::Logger::LOGGER] = loggerJSON;
}

void ConfigWriter::WriteWalletConfig(Json::Value& root, const std::string& databaseType) const
{
	Json::Value walletJSON;

	walletJSON[ConfigProps::Wallet::DATABASE] = databaseType;

	root[ConfigProps::Wallet::WALLET] = walletJSON;
}