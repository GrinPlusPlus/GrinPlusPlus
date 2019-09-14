#include "OrphanPool.h"

OrphanPool::OrphanPool() : m_orphanHeadersByHash(128)
{

}


bool OrphanPool::IsOrphan(const uint64_t height, const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	auto heightIter = m_orphansByHeight.find(height);
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetBlock().GetHash() == hash)
			{
				return true;
			}
		}
	}

	return false;
}

void OrphanPool::AddOrphanBlock(const FullBlock& block)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const BlockHeader& header = block.GetBlockHeader();
	if (!m_orphanHeadersByHash.contains(header.GetHash()))
	{
		m_orphanHeadersByHash.insert(header.GetHash(), header);
	}

	auto heightIter = m_orphansByHeight.find(block.GetBlockHeader().GetHeight());
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetBlock().GetHash() == block.GetHash())
			{
				return;
			}
		}

		heightIter->second.emplace_back(Orphan(block));
	}
	else
	{
		m_orphansByHeight[block.GetBlockHeader().GetHeight()] = std::vector<Orphan>({ Orphan(block) });
	}
}

std::unique_ptr<FullBlock> OrphanPool::GetOrphanBlock(const uint64_t height, const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	auto heightIter = m_orphansByHeight.find(height);
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetBlock().GetHash() == hash)
			{
				FullBlock block = orphanIter->GetBlock();
				return std::make_unique<FullBlock>(std::move(block));
			}
		}
	}

	return std::unique_ptr<FullBlock>(nullptr);
}

void OrphanPool::RemoveOrphan(const uint64_t height, const Hash& hash)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	auto iter = m_orphansByHeight.find(height);
	if (iter != m_orphansByHeight.end())
	{
		for (auto orphanIter = iter->second.begin(); orphanIter != iter->second.end(); orphanIter++)
		{
			if (orphanIter->GetBlock().GetHash() == hash)
			{
				iter->second.erase(orphanIter);
				break;
			}
		}

		if (iter->second.empty())
		{
			m_orphansByHeight.erase(height);
		}
	}
}

std::unique_ptr<BlockHeader> OrphanPool::GetOrphanHeader(const Hash& hash) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	auto iter = m_orphanHeadersByHash.find(hash);
	if (iter != m_orphanHeadersByHash.cend())
	{
		BlockHeader header = iter->value();
		return std::make_unique<BlockHeader>(std::move(header));
	}

	return std::unique_ptr<BlockHeader>(nullptr);
}

void OrphanPool::AddOrphanHeader(const BlockHeader& header)
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	m_orphanHeadersByHash.insert(header.GetHash(), header);
}