#include "MMRHashUtil.h"
#include "MMRUtil.h"
#include "LeafSet.h"

#include <Crypto/Hasher.h>
#include <Core/Serialization/Serializer.h>

void MMRHashUtil::AddHashes(
	const HashFile::Ptr& pHashFile,
	const std::vector<uint8_t>& serializedLeaf,
	const PruneList::CPtr& pPruneList)
{
	// Calculate next position
	uint64_t position = pHashFile->GetSize();
	if (pPruneList != nullptr) {
		position += pPruneList->GetTotalShift();
	}

	// Add in the new leaf hash
	const Hash leafHash = HashLeafWithIndex(serializedLeaf, position);
	pHashFile->AddData(leafHash);

	// Add parent hashes
	for (Index mmr_idx = Index::At(position + 1); !mmr_idx.IsLeaf(); mmr_idx++) {
		const Hash leftHash = GetHashAt(pHashFile, mmr_idx.GetLeftChild(), pPruneList);
		const Hash rightHash = GetHashAt(pHashFile, mmr_idx.GetRightChild(), pPruneList);

		const Hash parentHash = HashParentWithIndex(leftHash, rightHash, mmr_idx.Get());
		pHashFile->AddData(parentHash);
	}
}

Hash MMRHashUtil::Root(
	const HashFile::CPtr& pHashFile,
	const uint64_t size,
	const PruneList::CPtr& pPruneList)
{
	if (size == 0) {
		return ZERO_HASH;
	}

	Hash hash = ZERO_HASH;
	const std::vector<uint64_t> peakIndices = MMRUtil::GetPeakIndices(size);
	for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++) {
		const uint64_t shiftedIndex = GetShiftedIndex(Index::At(*iter), pPruneList);
		Hash peakHash = pHashFile->GetDataAt(shiftedIndex);
		if (peakHash != ZERO_HASH) {
			if (hash == ZERO_HASH) {
				hash = peakHash;
			} else {
				hash = HashParentWithIndex(peakHash, hash, size);
			}
		}
	}

	return hash;
}

Hash MMRHashUtil::GetHashAt(
	const HashFile::CPtr& pHashFile,
	const Index& mmr_idx,
	const PruneList::CPtr& pPruneList)
{
	if (pPruneList != nullptr) {
		if (pPruneList->IsCompacted(mmr_idx)) {
			return ZERO_HASH;
		}

		return pHashFile->GetDataAt(mmr_idx.Get() - pPruneList->GetShift(mmr_idx));
	} else {
		return pHashFile->GetDataAt(mmr_idx.Get());
	}
}

uint64_t MMRHashUtil::GetShiftedIndex(const Index& mmr_idx, const PruneList::CPtr& pPruneList)
{
	if (pPruneList != nullptr) {
		return mmr_idx.Get() - pPruneList->GetShift(mmr_idx);
	} else {
		return mmr_idx.Get();
	}
}

std::vector<Hash> MMRHashUtil::GetLastLeafHashes(
	const HashFile::CPtr& pHashFile,
	const std::shared_ptr<const LeafSet>& pLeafSet,
	const PruneList::CPtr& pPruneList,
	const uint64_t numHashes)
{
	uint64_t nextPosition = pHashFile->GetSize();
	if (pPruneList != nullptr) {
		nextPosition += pPruneList->GetTotalShift();
	}

	std::vector<Hash> hashes;

	LeafIndex leaf_idx = LeafIndex::AtPos(nextPosition);
	while (leaf_idx > (uint64_t)0 && hashes.size() < numHashes) {
		--leaf_idx;

		if (pLeafSet == nullptr || pLeafSet->Contains(leaf_idx)) {
			Hash hash = GetHashAt(pHashFile, leaf_idx.GetIndex(), pPruneList);
			if (hash != ZERO_HASH) {
				hashes.emplace_back(std::move(hash));
			}
		}
	}

	return hashes;
}

Hash MMRHashUtil::HashLeafWithIndex(const std::vector<unsigned char>& serializedLeaf, const uint64_t mmrIndex)
{
	Serializer hashSerializer;
	hashSerializer.Append<uint64_t>(mmrIndex);
	hashSerializer.AppendByteVector(serializedLeaf);
	return Hasher::Blake2b(hashSerializer.GetBytes());
}

Hash MMRHashUtil::HashParentWithIndex(const Hash& leftChild, const Hash& rightChild, const uint64_t parentIndex)
{
	Serializer serializer;
	serializer.Append<uint64_t>(parentIndex);
	serializer.AppendBigInteger<32>(leftChild);
	serializer.AppendBigInteger<32>(rightChild);
	return Hasher::Blake2b(serializer.GetBytes());
}