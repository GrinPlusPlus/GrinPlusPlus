#pragma once

#include "CRoaring/roaring.hh"

#include <string>
#include <vector>
#include <stdint.h>

// TODO: Document this.
class PruneList
{
public:
	static PruneList Load(const std::string& filePath);

	bool Flush();

	void Add(const uint64_t mmrIndex);
	bool IsPruned(const uint64_t mmrIndex) const;
	bool IsPrunedRoot(const uint64_t mmrIndex) const;

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