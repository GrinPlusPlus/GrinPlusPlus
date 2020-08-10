#include "OrphanPool.h"

OrphanPool::OrphanPool() : m_orphanHeadersByHash(64)
{

}

bool OrphanPool::IsOrphan(const uint64_t height, const Hash& hash) const
{
	auto heightIter = m_orphansByHeight.find(height);
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetHash() == hash)
			{
				return true;
			}
		}
	}

	return false;
}

void OrphanPool::AddOrphanBlock(const FullBlock& block)
{
	if (!m_orphanHeadersByHash.Cached(block.GetHash()))
	{
		m_orphanHeadersByHash.Put(block.GetHash(), block.GetHeader());
	}

	auto heightIter = m_orphansByHeight.find(block.GetHeight());
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetHash() == block.GetHash())
			{
				return;
			}
		}

		heightIter->second.emplace_back(Orphan(block));
	}
	else
	{
		m_orphansByHeight[block.GetHeight()] = std::vector<Orphan>({ Orphan(block) });
	}
}

std::shared_ptr<const FullBlock> OrphanPool::GetOrphanBlock(const uint64_t height, const Hash& hash) const
{
	auto heightIter = m_orphansByHeight.find(height);
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetHash() == hash)
			{
				return orphanIter->GetBlock();
			}
		}
	}

	return std::shared_ptr<const FullBlock>(nullptr);
}

std::shared_ptr<const FullBlock> OrphanPool::GetNextOrphanBlock(const uint64_t height, const Hash& previousHash) const
{
	auto heightIter = m_orphansByHeight.find(height);
	if (heightIter != m_orphansByHeight.cend())
	{
		for (auto orphanIter = heightIter->second.cbegin(); orphanIter != heightIter->second.cend(); orphanIter++)
		{
			if (orphanIter->GetBlock()->GetPreviousHash() == previousHash)
			{
				return orphanIter->GetBlock();
			}
		}
	}

	return std::shared_ptr<const FullBlock>(nullptr);
}

void OrphanPool::RemoveOrphan(const uint64_t height, const Hash& hash)
{
	auto iter = m_orphansByHeight.find(height);
	if (iter != m_orphansByHeight.end())
	{
		for (auto orphanIter = iter->second.begin(); orphanIter != iter->second.end(); orphanIter++)
		{
			if (orphanIter->GetHash() == hash)
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

BlockHeaderPtr OrphanPool::GetOrphanHeader(const Hash& hash) const
{
	if (m_orphanHeadersByHash.Cached(hash))
	{
		return m_orphanHeadersByHash.Get(hash);
	}

	return nullptr;
}

void OrphanPool::AddOrphanHeader(BlockHeaderPtr pHeader)
{
	m_orphanHeadersByHash.Put(pHeader->GetHash(), pHeader);
}