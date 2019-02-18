#include "LeafSet.h"
#include "MMRUtil.h"

#include <fstream>
#include <Common/Util/FileUtil.h>
#include <Common/Util/HexUtil.h>

LeafSet::LeafSet(const std::string& path)
	: m_path(path)
{

}

void LeafSet::Add(const uint32_t position)
{
	m_bitmap.add(position + 1);
}

void LeafSet::Remove(const uint32_t position)
{
	m_bitmap.remove(position + 1);
}

void LeafSet::Rewind(const uint64_t size, const Roaring& positionsToAdd)
{
	// Add back pruned positions.
	m_bitmap |= positionsToAdd;

	// Remove positions after the rewind point.
	Roaring positionsToRemove;
	positionsToRemove.addRange((uint32_t)size + 1, m_bitmap.maximum());
	m_bitmap -= positionsToRemove;
}

bool LeafSet::Contains(const uint64_t position) const
{
	return m_bitmap.contains((uint32_t)position + 1);
}

bool LeafSet::Load()
{
	std::vector<unsigned char> data;
	if (FileUtil::ReadFile(m_path, data))
	{
		m_bitmap = Roaring::readSafe((const char*)&data[0], data.size());
		m_bitmapBackup = m_bitmap;

		return true;
	}

	return false;
}

bool LeafSet::Flush()
{
	if (Flush(m_path, m_bitmap))
	{
		m_bitmapBackup = m_bitmap;
		return true;
	}

	return false;
}

bool LeafSet::Flush(const std::string& path, Roaring& bitmap) const
{
	bitmap.runOptimize();

	const size_t size = bitmap.getSizeInBytes();
	if (size > 0)
	{
		std::vector<unsigned char> buffer(size);
		if (size == bitmap.write((char*)&buffer[0]))
		{
			if (FileUtil::SafeWriteToFile(path, buffer))
			{
				return true;
			}
		}
	}

	return false;
}

bool LeafSet::Snapshot(const Hash& blockHash)
{
	std::string path = m_path + "." + HexUtil::ConvertHash(blockHash);
	Roaring snapshotBitmap = m_bitmap;

	return Flush(path, snapshotBitmap);
}

void LeafSet::Discard()
{
	m_bitmap = m_bitmapBackup;
}

// Calculate the set of pruned positions up to the cutoff size.
// Uses both the LeafSet and the PruneList to determine prunedness.
Roaring LeafSet::CalculatePrunedPositions(const uint64_t cutoffSize, const Roaring& rewindRmPos, const PruneList& pruneList) const
{
	Roaring bitmap = m_bitmap;

	// First remove positions from LeafSet that were added after the point we are rewinding to.
	Roaring rewindAddedPositions;
	rewindAddedPositions.addRange(cutoffSize, m_bitmap.maximum());
	bitmap -= rewindAddedPositions;

	// Then add back output positions to the LeafSet that were removed.
	bitmap |= rewindRmPos;

	// Invert bitmap for the leaf pos and return the resulting bitmap.
	const Roaring unpruned = CalculateUnprunedPositions(cutoffSize, pruneList);
	bitmap.flip(0, cutoffSize);
	
	return bitmap & unpruned;
}

// Calculate the set of unpruned leaves up to the cutoff size.
Roaring LeafSet::CalculateUnprunedPositions(const uint64_t cutoffSize, const PruneList& pruneList) const
{
	std::vector<uint32_t> unprunedPositions; // TODO: Reserve size for performance hint?
	for (uint64_t i = 0; i < cutoffSize; i++)
	{
		if (MMRUtil::IsLeaf(i) && !pruneList.IsPruned(i))
		{
			unprunedPositions.push_back((uint32_t)i + 1);
		}
	}

	return Roaring(unprunedPositions.size(), &unprunedPositions[0]);
}