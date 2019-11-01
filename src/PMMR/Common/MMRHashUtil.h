#pragma once

#include "HashFile.h"
#include "LeafSet.h"
#include "PruneList.h"

#include <Core/Traits/Lockable.h>
#include <vector>

class MMRHashUtil
{
public:
	// TODO: Take in std::shared_ptr<HashFile> instead of Writer/Reader
	static void AddHashes(std::shared_ptr<HashFile> pHashFile, const std::vector<unsigned char>& serializedLeaf, const PruneList* pPruneList);
	static Hash Root(std::shared_ptr<const HashFile> pHashFile, const uint64_t size, const PruneList* pPruneList);
	static Hash GetHashAt(std::shared_ptr<const HashFile> pHashFile, const uint64_t mmrIndex, const PruneList* pPruneList);
	static std::vector<Hash> GetLastLeafHashes(std::shared_ptr<const HashFile> pHashFile, const LeafSet* pLeafSet, const PruneList* pPruneList, const uint64_t numHashes);
	static Hash HashParentWithIndex(const Hash& leftChild, const Hash& rightChild, const uint64_t parentIndex);

private:
	static Hash HashLeafWithIndex(const std::vector<unsigned char>& serializedLeaf, const uint64_t mmrIndex);
	static uint64_t GetShiftedIndex(const uint64_t mmrIndex, const PruneList* pPruneList);
};