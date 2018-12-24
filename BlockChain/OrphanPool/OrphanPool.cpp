#include "OrphanPool.h"

bool OrphanPool::IsOrphan(const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	return m_orphans.find(hash) != m_orphans.cend();
}

void OrphanPool::AddOrphanBlock(const FullBlock& block)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	if (m_orphans.find(block.GetHash()) == m_orphans.cend())
	{
		m_orphans[block.GetHash()] = new Orphan(block);
	}
}

std::unique_ptr<FullBlock> OrphanPool::GetOrphanBlock(const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	auto iter = m_orphans.find(hash);
	if (iter != m_orphans.cend())
	{
		FullBlock block = iter->second->GetBlock();
		return std::make_unique<FullBlock>(std::move(block));
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

std::vector<FullBlock> OrphanPool::GetOrphanBlocks(const uint64_t height, const Hash& previousHash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<FullBlock> orphanBlocks;

	auto iter = m_orphansByHeight.find(height);
	if (iter != m_orphansByHeight.cend())
	{
		const std::vector<Orphan*> orphans = GetOrphansByHash_Locked(iter->second);
		for (const Orphan* pOrphan : orphans)
		{
			if (pOrphan->GetBlock().GetBlockHeader().GetPreviousBlockHash() == previousHash)
			{
				orphanBlocks.push_back(pOrphan->GetBlock());
			}
		}
	}

	return orphanBlocks;
}

std::vector<Orphan*> OrphanPool::GetOrphansByHash_Locked(const std::set<Hash>& hashes) const
{
	std::vector<Orphan*> orphans;

	for (const Hash& hash : hashes)
	{
		auto orphanIter = m_orphans.find(hash);
		if (orphanIter != m_orphans.cend())
		{
			orphans.push_back(orphanIter->second);
		}
	}

	return orphans;
}

void OrphanPool::RemoveOrphan(const Hash& hash)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	auto iter = m_orphans.find(hash);
	if (iter != m_orphans.end())
	{
		const uint64_t height = iter->second->GetBlock().GetBlockHeader().GetHeight();
		RemoveHashAtHeight_Locked(height, hash);

		delete iter->second;
		m_orphans.erase(iter);
	}
}

void OrphanPool::RemoveHashAtHeight_Locked(const uint64_t height, const Hash& hash)
{
	auto iter = m_orphansByHeight.find(height);
	if (iter != m_orphansByHeight.end())
	{
		iter->second.erase(hash);

		if (iter->second.empty())
		{
			m_orphansByHeight.erase(height);
		}
	}
}