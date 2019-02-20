#pragma once

#include <Core/CRoaring/roaring.hh>

#include <string>
#include <vector>
#include <stdint.h>

class PruneList
{
public:
	static PruneList Load(const std::string& filePath);

	bool Flush();

	// Adds the node to the prune list.
	// Compacts if pruning the node means a parent can get pruned as well.
	void Add(const uint64_t mmrIndex);

	// Indicates whether the position is pruned.
	bool IsPruned(const uint64_t mmrIndex) const;

	// Indicates whether the position is the root of a pruned subtree.
	bool IsPrunedRoot(const uint64_t mmrIndex) const;

	bool IsCompacted(const uint64_t mmrIndex) const;

	uint64_t GetTotalShift() const;
	uint64_t GetShift(const uint64_t mmrIndex) const;
	uint64_t GetLeafShift(const uint64_t mmrIndex) const;

private:
	PruneList(const std::string& filePath, Roaring&& prunedRoots);

	void BuildPrunedCache();
	void BuildShiftCaches();

	const std::string m_filePath;

	Roaring m_prunedRoots;
	Roaring m_prunedCache;
	std::vector<uint64_t> m_shiftCache;
	std::vector<uint64_t> m_leafShiftCache;
};