#pragma once

#include "HashFile.h"
#include "PruneList.h"

#include <Crypto/Models/Hash.h>
#include <Core/Traits/Lockable.h>
#include <PMMR/Common/Index.h>
#include <vector>

// Forward Declarations
class LeafSet;

class MMRHashUtil
{
public:
	static void AddHashes(
		const HashFile::Ptr& pHashFile,
		const std::vector<uint8_t>& serializedLeaf,
		const PruneList::CPtr& pPruneList
	);

	static Hash Root(
		const HashFile::CPtr& pHashFile,
		const uint64_t size,
		const PruneList::CPtr& pPruneList
	);

	static Hash GetHashAt(
		const HashFile::CPtr& pHashFile,
		const Index& mmrIndex,
		const PruneList::CPtr& pPruneList
	);

	static std::vector<Hash> GetLastLeafHashes(
		const HashFile::CPtr& pHashFile,
		const std::shared_ptr<const LeafSet>& pLeafSet,
		const PruneList::CPtr& pPruneList,
		const uint64_t numHashes
	);

	static Hash HashParentWithIndex(const Hash& leftChild, const Hash& rightChild, const uint64_t parentIndex);

private:
	static Hash HashLeafWithIndex(const std::vector<uint8_t>& serializedLeaf, const uint64_t mmrIndex);
	static uint64_t GetShiftedIndex(const Index& mmr_idx, const PruneList::CPtr& pPruneList);
};