#pragma once

#include "HashFile.h"
#include "PruneList.h"

#include <Crypto/Models/Hash.h>
#include <Core/Traits/Lockable.h>
#include <vector>

// Forward Declarations
class LeafSet;

class MMRHashUtil
{
public:
	static void AddHashes(
		std::shared_ptr<HashFile> pHashFile,
		const std::vector<unsigned char>& serializedLeaf,
		std::shared_ptr<const PruneList> pPruneList
	);

	static Hash Root(
		std::shared_ptr<const HashFile> pHashFile,
		const uint64_t size,
		std::shared_ptr<const PruneList> pPruneList
	);

	static Hash GetHashAt(
		std::shared_ptr<const HashFile> pHashFile,
		const uint64_t mmrIndex,
		std::shared_ptr<const PruneList> pPruneList
	);

	static std::vector<Hash> GetLastLeafHashes(
		std::shared_ptr<const HashFile> pHashFile,
		std::shared_ptr<const LeafSet> pLeafSet,
		std::shared_ptr<const PruneList> pPruneList,
		const uint64_t numHashes
	);

	static Hash HashParentWithIndex(const Hash& leftChild, const Hash& rightChild, const uint64_t parentIndex);

private:
	static Hash HashLeafWithIndex(const std::vector<unsigned char>& serializedLeaf, const uint64_t mmrIndex);
	static uint64_t GetShiftedIndex(const uint64_t mmrIndex, std::shared_ptr<const PruneList> pPruneList);
};