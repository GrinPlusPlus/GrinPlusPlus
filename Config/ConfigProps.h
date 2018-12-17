#pragma once

#include <string.h>

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
}