#pragma once

#include "HashFile.h"
#include "LeafSet.h"
#include "PruneList.h"

#include <vector>

class MMRHashUtil
{
public:
	static void AddHashes(HashFile& hashFile, const std::vector<unsigned char>& serializedLeaf, const PruneList* pPruneList);
	static Hash Root(const HashFile& hashFile, const uint64_t size, const PruneList* pPruneList);
	static Hash GetHashAt(const HashFile& hashFile, const uint64_t mmrIndex, const PruneList* pPruneList);
	static std::vector<Hash> GetLastLeafHashes(const HashFile& hashFile, const LeafSet* pLeafSet, const PruneList* pPruneList, const uint64_t numHashes);
	static Hash HashParentWithIndex(const Hash& leftChild, const Hash& rightChild, const uint64_t parentIndex);

private:
	static Hash HashLeafWithIndex(const std::vector<unsigned char>& serializedLeaf, const uint64_t mmrIndex);
	static uint64_t GetShiftedIndex(const uint64_t mmrIndex, const PruneList* pPruneList);
};