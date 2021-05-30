#pragma once

#include <Roaring.h>
#include <PMMR/Common/Index.h>
#include <filesystem.h>

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

class PruneList
{
public:
	using Ptr = std::shared_ptr<PruneList>;
	using CPtr = std::shared_ptr<const PruneList>;

	static PruneList::Ptr Load(const fs::path& filePath);

	void Flush();

	// Adds the node to the prune list.
	// Compacts if pruning the node means a parent can get pruned as well.
	void Add(const Index& mmrIndex);

	// Indicates whether the position is pruned.
	bool IsPruned(const Index& mmrIndex) const;

	// Indicates whether the position is the root of a pruned subtree.
	bool IsPrunedRoot(const Index& mmrIndex) const;

	bool IsCompacted(const Index& mmrIndex) const;

	uint64_t GetTotalShift() const;
	uint64_t GetShift(const Index& mmrIndex) const;
	uint64_t GetLeafShift(const Index& mmr_idx) const;

private:
	PruneList(const fs::path& filePath, Roaring&& prunedRoots);

	void BuildPrunedCache();
	void BuildShiftCaches();

	fs::path m_filePath;

	Roaring m_prunedRoots;
	Roaring m_prunedCache;
	std::vector<uint64_t> m_shiftCache;
	std::vector<uint64_t> m_leafShiftCache;
};