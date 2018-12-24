#include "OutputPMMR.h"
#include "Common/MMRUtil.h"

#include <StringUtil.h>
#include <Infrastructure/Logger.h>

OutputPMMR::OutputPMMR(const Config& config, HashFile&& hashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<OUTPUT_SIZE>&& dataFile)
	: m_config(config),
	m_hashFile(std::move(hashFile)),
	m_leafSet(std::move(leafSet)),
	m_pruneList(std::move(pruneList)),
	m_dataFile(std::move(dataFile))
{

}

OutputPMMR* OutputPMMR::Load(const Config& config)
{
	HashFile hashFile(config.GetTxHashSetDirectory() + "output/pmmr_hash.bin");
	hashFile.Load();

	LeafSet leafSet(config.GetTxHashSetDirectory() + "output/pmmr_leaf.bin");
	leafSet.Load();

	PruneList pruneList(std::move(PruneList::Load(config.GetTxHashSetDirectory() + "output/pmmr_prun.bin")));

	DataFile<OUTPUT_SIZE> dataFile(config.GetTxHashSetDirectory() + "output/pmmr_data.bin");
	dataFile.Load();

	return new OutputPMMR(config, std::move(hashFile), std::move(leafSet), std::move(pruneList), std::move(dataFile));
}

Hash OutputPMMR::Root(const uint64_t size) const
{
	LoggerAPI::LogTrace("OutputPMMR::Root - Calculating root of MMR with size " + std::to_string(size));

	if (size == 0)
	{
		return ZERO_HASH;
	}

	Hash hash = ZERO_HASH;
	const std::vector<uint64_t> peakIndices = MMRUtil::GetPeakIndices(size);
	for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++)
	{
		std::unique_ptr<Hash> pHash = GetHashAt(*iter);
		if (pHash != nullptr)
		{
			if (hash == ZERO_HASH)
			{
				hash = *pHash;
			}
			else
			{
				hash = MMRUtil::HashParentWithIndex(*pHash, hash, size);
			}
		}
	}

	return hash;
}

std::unique_ptr<Hash> OutputPMMR::GetHashAt(const uint64_t mmrIndex) const
{
	if (m_pruneList.IsPruned(mmrIndex) && !m_pruneList.IsPrunedRoot(mmrIndex))
	{
		return std::unique_ptr<Hash>(nullptr);
	}

	const uint64_t shift = m_pruneList.GetShift(mmrIndex);
	const uint64_t shiftedIndex = (mmrIndex - shift);

	return std::make_unique<Hash>(m_hashFile.GetHashAt(shiftedIndex));
}

std::unique_ptr<OutputIdentifier> OutputPMMR::GetOutputAt(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		if (m_leafSet.Contains(mmrIndex))
		{
			if (m_pruneList.IsPruned(mmrIndex) && !m_pruneList.IsPrunedRoot(mmrIndex))
			{
				return std::unique_ptr<OutputIdentifier>(nullptr);
			}

			const uint64_t shift = m_pruneList.GetLeafShift(mmrIndex);
			const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);
			const uint64_t shiftedIndex = ((numLeaves - 1) - shift);

			std::vector<unsigned char> data;
			m_dataFile.GetDataAt(shiftedIndex, data);

			if (data.size() == OUTPUT_SIZE)
			{
				ByteBuffer byteBuffer(data);
				return std::make_unique<OutputIdentifier>(OutputIdentifier::Deserialize(byteBuffer));
			}
		}
	}

	return std::unique_ptr<OutputIdentifier>(nullptr);
}

uint64_t OutputPMMR::GetSize() const
{
	const uint64_t totalShift = m_pruneList.GetTotalShift();

	return totalShift + m_hashFile.GetSize();
}

bool OutputPMMR::Rewind(const uint64_t lastMMRIndex)
{
	const bool hashRewind = m_hashFile.Rewind(lastMMRIndex - m_pruneList.GetShift(lastMMRIndex));
	const bool dataRewind = m_dataFile.Rewind(MMRUtil::GetNumLeaves(lastMMRIndex) - m_pruneList.GetLeafShift(lastMMRIndex));
	// TODO: Rewind leafset

	return hashRewind && dataRewind;
}

bool OutputPMMR::Flush()
{
	LoggerAPI::LogInfo(StringUtil::Format("OutputPMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_hashFile.Flush();
	const bool dataFlush = m_dataFile.Flush();
	const bool leafSetFlush = m_leafSet.Flush();
	const bool pruneFlush = m_pruneList.Flush();

	return hashFlush && dataFlush && leafSetFlush && pruneFlush;
}

Roaring OutputPMMR::DetermineLeavesToRemove(const uint64_t cutoffSize, const Roaring& rewindRmPos) const
{	
	return m_leafSet.CalculatePrunedPositions(cutoffSize, rewindRmPos, m_pruneList);
}

struct IteratorParam
{
	IteratorParam(Roaring& expanded, const PruneList& pruneList)
		: expanded(expanded), pruneList(pruneList)
	{

	}

	Roaring& expanded;
	const PruneList& pruneList;
};

bool Iterator(const uint32_t value, void* param)
{
	IteratorParam* pIteratorParam = (IteratorParam*)param;
	Roaring& expanded = pIteratorParam->expanded;
	const PruneList& pruneList = pIteratorParam->pruneList;

	expanded.add(value);
	uint64_t current = value;

	while (true)
	{
		const uint64_t siblingIndex = MMRUtil::GetSiblingIndex(current);

		const bool siblingPruned = pruneList.IsPrunedRoot(siblingIndex);

		// if sibling previously pruned, push it back onto list of pos to remove so we can remove it and traverse up to parent
		if (siblingPruned)
		{
			expanded.add(siblingIndex);
		}

		if (siblingPruned || expanded.contains(siblingIndex))
		{
			const uint64_t parentIndex = MMRUtil::GetParentIndex(current);
			expanded.add(parentIndex);
			current = parentIndex;
		}
		else
		{
			break;
		}
	}

	return true;
}

Roaring OutputPMMR::DetermineNodesToRemove(const Roaring& leavesToRemove) const
{
	Roaring expanded;

	IteratorParam iteratorParam(expanded, m_pruneList);
	leavesToRemove.iterate(Iterator, &iteratorParam);

	return expanded;
}