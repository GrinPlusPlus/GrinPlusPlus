#pragma once

#include "HashFile.h"
#include "PruneList.h"

#include <vector>

class MMRHashUtil
{
public:
	static void AddHashes(HashFile& hashFile, const std::vector<unsigned char>& serializedLeaf, const PruneList* pPruneList);
	static Hash Root(const HashFile& hashFile, const uint64_t size, const PruneList* pPruneList);
	static Hash GetHashAt(const HashFile& hashFile, const uint64_t mmrIndex, const PruneList* pPruneList);
	static Hash HashParentWithIndex(const Hash& leftChild, const Hash& rightChild, const uint64_t parentIndex);

private:
	static Hash HashLeafWithIndex(const std::vector<unsigned char>& serializedLeaf, const uint64_t mmrIndex);
};