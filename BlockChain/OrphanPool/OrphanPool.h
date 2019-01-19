#pragma once

#include "Orphan.h"

#include <Hash.h>
#include <Core/FullBlock.h>
#include <unordered_map>
#include <map>
#include <shared_mutex>

class OrphanPool
{
public:
	bool IsOrphan(const uint64_t height, const Hash& hash) const;
	void AddOrphanBlock(const FullBlock& block);
	std::unique_ptr<FullBlock> GetOrphanBlock(const uint64_t height, const Hash& hash) const;
	std::vector<FullBlock> GetOrphanBlocks(const uint64_t height, const Hash& previousHash) const;
	void RemoveOrphan(const uint64_t height, const Hash& hash);

private:
	mutable std::shared_mutex m_mutex;
	std::unordered_map<uint64_t, std::vector<Orphan>> m_orphansByHeight;
};