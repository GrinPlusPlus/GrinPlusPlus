#pragma once

#include <Crypto/Models/Hash.h>
#include <PMMR/Common/Index.h>
#include <cstdint>
#include <memory>

class MMR
{
public:
	virtual ~MMR() = default;

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
	virtual std::unique_ptr<Hash> GetHashAt(const Index& mmrIndex) const = 0;

	//
	// Gets the last n leaf hashes.
	//
	virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const = 0;

	//
	// Flushes all working changes to disk.
	//
	virtual void Commit() = 0;

	//
	// Discards all working changes since the last flush to disk.
	//
	virtual void Rollback() noexcept = 0;
};