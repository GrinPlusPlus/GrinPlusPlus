#pragma once

#include "Orphan.h"

#include <Core/Models/FullBlock.h>
#include <Crypto/Hash.h>
#include <lru/cache.hpp>
#include <shared_mutex>
#include <unordered_map>

class OrphanPool
{
  public:
    OrphanPool();

    bool IsOrphan(const uint64_t height, const Hash &hash) const;
    void AddOrphanBlock(const FullBlock &block);
    std::unique_ptr<FullBlock> GetOrphanBlock(const uint64_t height, const Hash &hash) const;
    void RemoveOrphan(const uint64_t height, const Hash &hash);

    void AddOrphanHeader(const BlockHeader &header);
    std::unique_ptr<BlockHeader> GetOrphanHeader(const Hash &hash) const;

  private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<uint64_t, std::vector<Orphan>> m_orphansByHeight;
    LRU::Cache<Hash, BlockHeader> m_orphanHeadersByHash;
};