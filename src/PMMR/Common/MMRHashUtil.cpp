#include "MMRHashUtil.h"
#include "MMRUtil.h"
#include "LeafSet.h"

#include <Crypto/Hasher.h>
#include <Core/Serialization/Serializer.h>

void MMRHashUtil::AddHashes(
	std::shared_ptr<HashFile> pHashFile,
	const std::vector<unsigned char>& serializedLeaf,
	std::shared_ptr<const PruneList> pPruneList)
{
	// Calculate next position
	uint64_t position = pHashFile->GetSize();
	if (pPruneList != nullptr)
	{
		position += pPruneList->GetTotalShift();
	}

	// Add in the new leaf hash
	const Hash leafHash = HashLeafWithIndex(serializedLeaf, position);
	pHashFile->AddData(leafHash);

	// Add parent hashes
	uint64_t peak = 1;
	while (MMRUtil::GetHeight(position + 1) > 0)
	{
		const uint64_t leftSiblingPosition = (position + 1) - (2 * peak);

		const Hash leftHash = GetHashAt(pHashFile, leftSiblingPosition, pPruneList);
		const Hash rightHash = GetHashAt(pHashFile, position, pPruneList);

		++position;
		peak *= 2;

		const Hash parentHash = HashParentWithIndex(leftHash, rightHash, position);
		pHashFile->AddData(parentHash);
	}
}

Hash MMRHashUtil::Root(
	std::shared_ptr<const HashFile> pHashFile,
	const uint64_t size,
	std::shared_ptr<const PruneList> pPruneList)
{
	if (size == 0)
	{
		return ZERO_HASH;
	}

	Hash hash = ZERO_HASH;
	const std::vector<uint64_t> peakIndices = MMRUtil::GetPeakIndices(size);
	for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++)
	{
		const uint64_t shiftedIndex = GetShiftedIndex(*iter, pPruneList);
		Hash peakHash = pHashFile->GetDataAt(shiftedIndex);
		if (peakHash != ZERO_HASH)
		{
			if (hash == ZERO_HASH)
			{
				hash = peakHash;
			}
			else
			{
				hash = HashParentWithIndex(peakHash, hash, size);
			}
		}
	}

	return hash;
}

Hash MMRHashUtil::GetHashAt(
	std::shared_ptr<const HashFile> pHashFile,
	const uint64_t mmrIndex,
	std::shared_ptr<const PruneList> pPruneList)
{
	if (pPruneList != nullptr)
	{
		if (pPruneList->IsCompacted(mmrIndex))
		{
			return ZERO_HASH;
		}

		const uint64_t shift = pPruneList->GetShift(mmrIndex);
		const uint64_t shiftedIndex = (mmrIndex - shift);

		return pHashFile->GetDataAt(shiftedIndex);
	}
	else
	{
		return pHashFile->GetDataAt(mmrIndex);
	}
}

uint64_t MMRHashUtil::GetShiftedIndex(const uint64_t mmrIndex, std::shared_ptr<const PruneList> pPruneList)
{
	if (pPruneList != nullptr)
	{
		const uint64_t shift = pPruneList->GetShift(mmrIndex);

		return mmrIndex - shift;
	}
	else
	{
		return mmrIndex;
	}
}

std::vector<Hash> MMRHashUtil::GetLastLeafHashes(
	std::shared_ptr<const HashFile> pHashFile,
	std::shared_ptr<const LeafSet> pLeafSet,
	std::shared_ptr<const PruneList> pPruneList,
	const uint64_t numHashes)
{
	uint64_t nextPosition = pHashFile->GetSize();
	if (pPruneList != nullptr)
	{
		nextPosition += pPruneList->GetTotalShift();
	}

	std::vector<Hash> hashes;

	uint64_t leafIndex =  MMRUtil::GetNumNodes(nextPosition);
	while (leafIndex > 0 && hashes.size() < numHashes)
	{
		--leafIndex;

		if (pLeafSet == nullptr || pLeafSet->Contains(leafIndex))
		{
			const uint64_t mmrIndex = MMRUtil::GetPMMRIndex(leafIndex);
			Hash hash = GetHashAt(pHashFile, mmrIndex, pPruneList);
			if (hash != ZERO_HASH)
			{
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