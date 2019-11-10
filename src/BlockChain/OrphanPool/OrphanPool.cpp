#include "OrphanPool.h"

OrphanPool::OrphanPool() : m_orphanHeadersByHash(128)
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
	if (!m_orphanHeadersByHash.contains(block.GetHash()))
	{
		m_orphanHeadersByHash.insert(block.GetHash(), std::make_shared<const BlockHeader>(block.GetBlockHeader()));
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

std::shared_ptr<const BlockHeader> OrphanPool::GetOrphanHeader(const Hash& hash) const
{
	auto iter = m_orphanHeadersByHash.find(hash);
	if (iter != m_orphanHeadersByHash.cend())
	{
		return iter->value();
	}

	return std::shared_ptr<const BlockHeader>(nullptr);
}

void OrphanPool::AddOrphanHeader(const BlockHeader& header)
{
	m_orphanHeadersByHash.insert(header.GetHash(), std::make_shared<const BlockHeader>(header));
}