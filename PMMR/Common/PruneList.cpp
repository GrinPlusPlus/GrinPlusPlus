#include "PruneList.h"
#include "MMRUtil.h"

#include <Common/Util/FileUtil.h>

PruneList::PruneList(const std::string& filePath, Roaring&& prunedRoots)
	: m_filePath(filePath), m_prunedRoots(std::move(prunedRoots))
{

}

PruneList PruneList::Load(const std::string& filePath)
{
	std::vector<unsigned char> data;
	if (FileUtil::ReadFile(filePath, data))
	{
		Roaring prunedRoots = Roaring::readSafe((const char*)&data[0], data.size());
		PruneList pruneList(filePath, std::move(prunedRoots));
		pruneList.BuildPrunedCache();
		pruneList.BuildShiftCaches();

		return pruneList;
	}
	else
	{
		return PruneList(filePath, std::move(Roaring()));
	}
}

bool PruneList::Flush()
{
	// Run the optimization step on the bitmap.
	m_prunedRoots.runOptimize();

	// Write the updated bitmap file to disk.
	const size_t size = m_prunedRoots.getSizeInBytes();
	if (size > 0)
	{
		std::vector<unsigned char> buffer(size);
		m_prunedRoots.write((char*)&buffer[0]);

		const bool flushed = FileUtil::SafeWriteToFile(m_filePath, buffer);

		// Rebuild our "shift caches" here as we are flushing changes to disk
		// and the contents of our prune_list has likely changed.
		BuildPrunedCache();
		BuildShiftCaches();
		
		return flushed;
	}

	return false;
}

// Push the node at the provided position in the prune list.
// Compacts the list if pruning the additional node means a parent can get pruned as well.
void PruneList::Add(const uint64_t position)
{
	uint64_t currentIndex = position;
	while (true)
	{
		const uint64_t siblingIndex = MMRUtil::GetSiblingIndex(currentIndex);
		if (m_prunedRoots.contains(siblingIndex + 1) || m_prunedCache.contains(siblingIndex + 1))
		{
			m_prunedCache.add(currentIndex + 1);
			m_prunedRoots.remove(siblingIndex + 1);
			currentIndex = MMRUtil::GetParentIndex(currentIndex);
		}
		else
		{
			m_prunedCache.add(currentIndex + 1);
			m_prunedRoots.add(currentIndex + 1);
			break;
		}
	}
}

bool PruneList::IsPruned(const uint64_t position) const
{
	return m_prunedCache.contains(position + 1);
}

bool PruneList::IsPrunedRoot(const uint64_t position) const
{
	return m_prunedRoots.contains(position + 1);
}

bool PruneList::IsCompacted(const uint64_t position) const
{
	return IsPruned(position) && !IsPrunedRoot(position);
}

uint64_t PruneList::GetTotalShift() const
{
	return GetShift(m_prunedRoots.maximum());
}

uint64_t PruneList::GetShift(const uint64_t position) const
{
	if (m_prunedRoots.isEmpty())
	{
		return 0;
	}

	if (m_shiftCache.empty())
	{
		return 0;
	}

	const uint64_t index = m_prunedRoots.rank(position + 1);
	if (index == 0)
	{
		return 0;
	}

	if (index > m_shiftCache.size())
	{
		return m_shiftCache.back();
	}
	else
	{
		return m_shiftCache[index - 1];
	}
}

uint64_t PruneList::GetLeafShift(const uint64_t position) const
{
	if (m_prunedRoots.isEmpty())
	{
		return 0;
	}

	const uint64_t index = m_prunedRoots.rank(position + 1);
	if (index == 0)
	{
		return 0;
	}

	if (m_leafShiftCache.empty())
	{
		return 0;
	}

	if (index > m_leafShiftCache.size())
	{
		return m_leafShiftCache.back();
	}
	else
	{
		return m_leafShiftCache[index - 1];
	}
}

void PruneList::BuildPrunedCache()
{
	if (m_prunedRoots.isEmpty())
	{
		return;
	}

	const uint64_t maximum = m_prunedRoots.maximum();
	m_prunedCache = Roaring(roaring_bitmap_create_with_capacity(maximum));
	for (uint64_t position = 0; position < maximum; position++)
	{
		uint64_t parent = position;
		while (parent < maximum)
		{
			if (m_prunedRoots.contains(parent + 1))
			{
				m_prunedCache.add(position + 1);
				break;
			}

			parent = MMRUtil::GetParentIndex(parent);
		}
	}

	m_prunedCache.runOptimize();
}

void PruneList::BuildShiftCaches()
{
	if (m_prunedRoots.isEmpty())
	{
		return;
	}

	m_shiftCache.clear();
	m_leafShiftCache.clear();

	// TODO: Use m_prunedRoots.iterate instead of looping through all values and checking contains.
	const uint64_t maximum = m_prunedRoots.maximum();
	for (uint64_t position = 0; position < maximum; position++)
	{
		if (m_prunedRoots.contains(position + 1))
		{
			const uint64_t height = MMRUtil::GetHeight(position);

			// Add to shift cache
			const uint64_t previousShift = GetShift(position - 1);
			const uint64_t currentShift = 2 * ((1 << height) - 1);
			m_shiftCache.push_back(previousShift + currentShift);

			// Add to leaf shift cache
			const uint64_t previousLeafShift = GetLeafShift(position - 1);
			const uint64_t currentLeafShift = (height == 0) ? 0 : 1 << height;
			m_leafShiftCache.push_back(previousLeafShift + currentLeafShift);
		}
	}
}