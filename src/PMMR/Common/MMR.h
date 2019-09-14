#pragma once

#include <Crypto/Hash.h>
#include <stdint.h>
#include <memory>

class MMR
{
public:
	//
	// Returns the unpruned size of the MMR.
	//
	virtual uint64_t GetSize() const = 0;

	//
	// Calculates the Root of the MMR with the given last mmr index.
	//
	virtual Hash Root(const uint64_t lastMMRIndex) const = 0;

	//
	// Gets the Hash at the mmr index.
	// Returns NULL if the node has been pruned.
	//
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const = 0;

	//
	// Gets the last n leaf hashes.
	//
	virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const = 0;

	//
	// Flushes all working changes to disk.
	//
	virtual bool Flush() = 0;

	//
	// Discards all working changes since the last flush to disk.
	//
	virtual bool Discard() = 0;
};