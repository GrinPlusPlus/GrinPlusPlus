#pragma once

#include <Consensus/BlockDifficulty.h>
#include <stdint.h>

// Minimal header information required for the Difficulty calculation to take place.
struct HeaderInfo
{
public:
	HeaderInfo(const uint64_t timestamp, const uint64_t difficulty, const uint32_t secondaryScaling, const bool secondary)
		: m_timestamp(timestamp), m_difficulty(difficulty), m_secondaryScaling(secondaryScaling), m_secondary(secondary)
	{

	}

	// Constructor from a timestamp and difficulty, setting a default secondary PoW factor.
	static HeaderInfo FromTimeAndDiff(const uint64_t timestamp, const uint64_t difficulty)
	{
		return HeaderInfo(timestamp, difficulty, Consensus::INITIAL_GRAPH_WEIGHT, true);
	}

	// Constructor from a difficulty and secondary factor, setting a default timestamp.
	static HeaderInfo FromDiffAndScaling(const uint64_t difficulty, const uint64_t secondaryScaling)
	{
		return HeaderInfo(1, difficulty, (uint32_t)secondaryScaling, true);
	}

	inline uint64_t GetTimestamp() const { return m_timestamp; }
	inline uint64_t GetDifficulty() const { return m_difficulty; }
	inline uint32_t GetSecondaryScaling() const { return m_secondaryScaling; }
	inline bool IsSecondary() const { return m_secondary; }

private:
	// Timestamp of the header, 1 when not used (returned info)
	uint64_t m_timestamp;

	// Network difficulty or next difficulty to use
	uint64_t m_difficulty;

	// Network secondary PoW factor or factor to use
	uint32_t m_secondaryScaling;

	// Whether the header is a secondary proof of work
	bool m_secondary;
};