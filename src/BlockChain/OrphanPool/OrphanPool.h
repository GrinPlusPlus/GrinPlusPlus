#pragma once

#include "Orphan.h"

#include <Crypto/Hash.h>
#include <Core/Models/FullBlock.h>
#include <unordered_map>
#include <lru/cache.hpp>

class OrphanPool
{
public:
	OrphanPool();

	bool IsOrphan(const uint64_t height, const Hash& hash) const;
	void AddOrphanBlock(const FullBlock& block);
	std::shared_ptr<const FullBlock> GetOrphanBlock(const uint64_t height, const Hash& hash) const;
	void RemoveOrphan(const uint64_t height, const Hash& hash);

	void AddOrphanHeader(const BlockHeader& header);
	std::shared_ptr<const BlockHeader> GetOrphanHeader(const Hash& hash) const;

private:
	std::unordered_map<uint64_t, std::vector<Orphan>> m_orphansByHeight;
	LRU::Cache<Hash, std::shared_ptr<const BlockHeader>> m_orphanHeadersByHash;
};