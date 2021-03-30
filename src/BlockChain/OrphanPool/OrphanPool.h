#pragma once

#include "Orphan.h"

#include <Crypto/Models/Hash.h>
#include <Core/Models/FullBlock.h>
#include <unordered_map>
#include <caches/Cache.h>

class OrphanPool
{
public:
	OrphanPool();

	bool IsOrphan(const uint64_t height, const Hash& hash) const;
	void AddOrphanBlock(const FullBlock& block);
	std::shared_ptr<const FullBlock> GetOrphanBlock(const uint64_t height, const Hash& hash) const;
	std::shared_ptr<const FullBlock> GetNextOrphanBlock(const uint64_t height, const Hash& previousHash) const;
	void RemoveOrphan(const uint64_t height, const Hash& hash);

	void AddOrphanHeader(BlockHeaderPtr pHeader);
	BlockHeaderPtr GetOrphanHeader(const Hash& hash) const;

private:
	std::unordered_map<uint64_t, std::vector<Orphan>> m_orphansByHeight;
	LRUCache<Hash, BlockHeaderPtr> m_orphanHeadersByHash;
};
