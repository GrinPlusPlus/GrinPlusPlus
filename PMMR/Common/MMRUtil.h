#pragma once

#include <stdint.h>
#include <vector>

class MMRUtil
{
public:
	static uint64_t GetHeight(const uint64_t mmrIndex);
	static uint64_t GetParentIndex(const uint64_t mmrIndex);
	static uint64_t GetSiblingIndex(const uint64_t mmrIndex);
	static uint64_t GetLeftChildIndex(const uint64_t mmrIndex, const uint64_t height);
	static uint64_t GetRightChildIndex(const uint64_t mmrIndex);
	static std::vector<uint64_t> GetPeakIndices(const uint64_t size);
	static uint64_t GetNumNodes(const uint64_t mmrIndex);
	static uint64_t GetNumLeaves(const uint64_t lastMMRIndex);
	static bool IsLeaf(const uint64_t mmrIndex);
	static uint64_t GetPMMRIndex(const uint64_t leafIndex);

private:
	static std::vector<uint64_t> GetPeakSizes(const uint64_t size);
};