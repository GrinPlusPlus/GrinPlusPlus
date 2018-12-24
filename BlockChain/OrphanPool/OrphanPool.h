#pragma once

#include "Orphan.h"

#include <Hash.h>
#include <Core/FullBlock.h>
#include <map>
#include <set>
#include <shared_mutex>

class OrphanPool
{
public:
	bool IsOrphan(const Hash& hash) const;
	void AddOrphanBlock(const FullBlock& block);
	std::unique_ptr<FullBlock> GetOrphanBlock(const Hash& hash) const;
	std::vector<FullBlock> GetOrphanBlocks(const uint64_t height, const Hash& previousHash) const;
	void RemoveOrphan(const Hash& hash);

private:
	std::vector<Orphan*> GetOrphansByHash_Locked(const std::set<Hash>& hashes) const;
	void RemoveHashAtHeight_Locked(const uint64_t height, const Hash& hash);

	mutable std::shared_mutex m_mutex;
	std::map<Hash, Orphan*> m_orphans;
	std::map<uint64_t, std::set<Hash>> m_orphansByHeight;
};