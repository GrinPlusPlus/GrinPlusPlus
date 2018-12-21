#pragma once

#include <vector>
#include <string>
#include <stdint.h>

#include <Consensus/BlockWeight.h>

namespace P2P
{
	// Current latest version of the protocol
	static const uint32_t PROTOCOL_VERSION = 1;

	// Grin's user agent with current version
	static const std::string USER_AGENT = "MW/C++ 0.0.1";

	// Magic number expected in the header of every message
	static const std::vector<unsigned char> MAGIC_BYTES = { 0x53, 0x35 };

	// Size in bytes of a message header
	static const uint64_t HEADER_LENGTH = 11;

	// Max theoretical size of a block filled with outputs.
	static const uint64_t MAX_BLOCK_SIZE = ((Consensus::MAX_BLOCK_WEIGHT / Consensus::BLOCK_OUTPUT_WEIGHT) * 708);

	// Maximum number of block headers a peer should ever send
	static const uint32_t MAX_BLOCK_HEADERS = 512;

	// Maximum number of block bodies a peer should ever ask for and send
	static const uint32_t MAX_BLOCK_BODIES = 16;

	// Maximum number of peer addresses a peer should ever send
	static const uint32_t MAX_PEER_ADDRS = 256;

	// Maximum number of block header hashes to send as part of a locator
	static const uint32_t MAX_LOCATORS = 20;

	// How long a banned peer should be banned for
	static const int64_t BAN_WINDOW = 10800;

	// The max peer count
	static const uint8_t PEER_MAX_COUNT = 25;

	// min preferred peer count
	static const uint8_t PEER_MIN_PREFERRED_COUNT = 8;

	// Default port number
	static const uint16_t DEFAULT_PORT = 13414;
}