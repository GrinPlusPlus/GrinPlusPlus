#pragma once

#include <string>

namespace ConfigProps
{
	static const std::string CLIENT_MODE = "CLIENT_MODE";
	static const std::string ENVIRONMENT = "ENVIRONMENT";
	static const std::string DATA_PATH = "DATA_PATH";

	namespace P2P
	{
		static const std::string P2P = "P2P";

		static const std::string MIN_PEERS = "MIN_PEERS";
		static const std::string MAX_PEERS = "MAX_PEERS";
	}

	namespace Dandelion
	{
		static const std::string DANDELION = "DANDELION";

		static const std::string RELAY_SECS = "RELAY_SECS";
		static const std::string EMBARGO_SECS = "EMBARGO_SECS";
		static const std::string PATIENCE_SECS = "PATIENCE_SECS";
		static const std::string STEM_PROBABILITY = "STEM_PROBABILITY";
	}
	
	namespace Server
	{
		static const std::string SERVER = "SERVER";

		static const std::string REST_API_PORT = "REST_API_PORT";
		static const std::string OWNER_API_PORT = "OWNER_API_PORT";
	}

	namespace Logger
	{
		static const std::string LOGGER = "LOGGER";

		static const std::string LOG_LEVEL = "LOG_LEVEL";
	}

	namespace Wallet
	{
		static const std::string WALLET = "WALLET";

		static const std::string MIN_CONFIRMATIONS = "MIN_CONFIRMATIONS";

		static const std::string REUSE_ADDRESS = "REUSE_ADDRESS";
	}

	namespace Tor
	{
		static const std::string TOR = "TOR";

		static const std::string ENABLE_TOR = "ENABLE_TOR";
		static const std::string SOCKS_PORT = "SOCKS_PORT";
		static const std::string CONTROL_PORT = "CONTROL_PORT";
		static const std::string PASSWORD = "PASSWORD";
		static const std::string HASHED_PASSWORD = "HASHED_PASSWORD";
	}

	namespace GrinJoin
	{
		static const std::string GRINJOIN = "GRINJOIN";

		static const std::string SECRET_KEY = "SECRET_KEY";
	}
}