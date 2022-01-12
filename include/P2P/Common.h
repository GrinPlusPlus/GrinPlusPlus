#pragma once

#include <string>
#include <cstdint>

#include <Consensus.h>
#include <GrinVersion.h>
#include <Core/Enums/ProtocolVersion.h>

namespace P2P
{
	// Current latest version of the protocol
	static const uint32_t PROTOCOL_VERSION = (uint32_t)ProtocolVersion::Local();

	// Grin's user agent with current version
	static const std::string USER_AGENT = "Grin++ (Android) " + GRINPP_VERSION;

	// Size in bytes of a message header
	static const uint64_t HEADER_LENGTH = 11;

	// Max theoretical size of a block filled with outputs.
	static const uint64_t MAX_BLOCK_SIZE = ((Consensus::MAX_BLOCK_WEIGHT / Consensus::OUTPUT_WEIGHT) * 708);

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

	// Number of seconds to wait before retrying to connect to peer
	static const uint32_t RETRY_WINDOW = 5 * 60;
}