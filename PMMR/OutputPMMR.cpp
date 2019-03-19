#include "OutputPMMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

OutputPMMR::OutputPMMR(IBlockDB& blockDB, HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<OUTPUT_SIZE>* pDataFile)
	: m_blockDB(blockDB),
	m_pHashFile(pHashFile),
	m_leafSet(std::move(leafSet)),
	m_pruneList(std::move(pruneList)),
	m_pDataFile(pDataFile)
{

}

OutputPMMR::~OutputPMMR()
{
	delete m_pHashFile;
	m_pHashFile = nullptr;

	delete m_pDataFile;
	m_pDataFile = nullptr;
}

OutputPMMR* OutputPMMR::Load(const std::string& txHashSetDirectory, IBlockDB& blockDB)
{
	HashFile* pHashFile = new HashFile(txHashSetDirectory + "output/pmmr_hash.bin");
	pHashFile->Load();

	LeafSet leafSet(txHashSetDirectory + "output/pmmr_leaf.bin");
	leafSet.Load();

	PruneList pruneList(PruneList::Load(txHashSetDirectory + "output/pmmr_prun.bin"));

	DataFile<OUTPUT_SIZE>* pDataFile = new DataFile<OUTPUT_SIZE>(txHashSetDirectory + "output/pmmr_data.bin");
	pDataFile->Load();

	return new OutputPMMR(blockDB, pHashFile, std::move(leafSet), std::move(pruneList), pDataFile);
}

bool OutputPMMR::Append(const OutputIdentifier& output, const uint64_t blockHeight)
{
	// Add to LeafSet
	const uint64_t totalShift = m_pruneList.GetTotalShift();
	const uint64_t mmrIndex = m_pHashFile->GetSize() + totalShift;
	m_leafSet.Add((uint32_t)mmrIndex);

	// Save output position
	m_blockDB.AddOutputPosition(output.GetCommitment(), OutputLocation(mmrIndex, blockHeight));

	// Add to data file
	Serializer serializer;
	output.Serialize(serializer);
	m_pDataFile->AddData(serializer.GetBytes());

	// Add hashes
	MMRHashUtil::AddHashes(*m_pHashFile, serializer.GetBytes(), &m_pruneList);

	if (!MMRUtil::IsLeaf(mmrIndex))
	{
		throw std::exception();
	}

	return true;
}

bool OutputPMMR::Remove(const uint64_t mmrIndex)
{
	LoggerAPI::LogTrace("OutputPMMR::Remove - Spending output at index " + std::to_string(mmrIndex));

	if (!MMRUtil::IsLeaf(mmrIndex))
	{
		LoggerAPI::LogWarning("OutputPMMR::Remove - Output is not a leaf " + std::to_string(mmrIndex));
		return false;
	}

	if (!m_leafSet.Contains(mmrIndex))
	{
		LoggerAPI::LogWarning("OutputPMMR::Remove - LeafSet does not contain output " + std::to_string(mmrIndex));
		return false;
	}

	m_leafSet.Remove((uint32_t)mmrIndex);

	return true;
}

Hash OutputPMMR::Root(const uint64_t size) const
{
	return MMRHashUtil::Root(*m_pHashFile, size, &m_pruneList);
}

std::unique_ptr<Hash> OutputPMMR::GetHashAt(const uint64_t mmrIndex) const
{
	Hash hash = MMRHashUtil::GetHashAt(*m_pHashFile, mmrIndex, &m_pruneList);
	if (hash == ZERO_HASH)
	{
		return std::unique_ptr<Hash>(nullptr);
	}

	return std::make_unique<Hash>(std::move(hash));
}

std::vector<Hash> OutputPMMR::GetLastLeafHashes(const uint64_t numHashes) const
{
	return MMRHashUtil::GetLastLeafHashes(*m_pHashFile, &m_leafSet, &m_pruneList, numHashes);
}

bool OutputPMMR::IsUnspent(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		if (mmrIndex < GetSize())
		{
			return m_leafSet.Contains(mmrIndex);
		}
	}

	return false;
}

std::unique_ptr<OutputIdentifier> OutputPMMR::GetOutputAt(const uint64_t mmrIndex) const
{
	if (IsUnspent(mmrIndex))
	{
		const uint64_t shift = m_pruneList.GetLeafShift(mmrIndex);
		const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);
		const uint64_t shiftedIndex = ((numLeaves - 1) - shift);

		std::vector<unsigned char> data;
		m_pDataFile->GetDataAt(shiftedIndex, data);

		if (data.size() == OUTPUT_SIZE)
		{
			ByteBuffer byteBuffer(data);
			return std::make_unique<OutputIdentifier>(OutputIdentifier::Deserialize(byteBuffer));
		}
	}

	return std::unique_ptr<OutputIdentifier>(nullptr);
}

uint64_t OutputPMMR::GetSize() const
{
	const uint64_t totalShift = m_pruneList.GetTotalShift();

	return totalShift + m_pHashFile->GetSize();
}

bool OutputPMMR::Rewind(const uint64_t size, const std::optional<Roaring>& leavesToAddOpt)
{
	const bool hashRewind = m_pHashFile->Rewind(size - m_pruneList.GetShift(size - 1));
	const bool dataRewind = m_pDataFile->Rewind(MMRUtil::GetNumLeaves(size - 1) - m_pruneList.GetLeafShift(size - 1));
	if (leavesToAddOpt.has_value())
	{
		m_leafSet.Rewind(size, leavesToAddOpt.value());
	}

	return hashRewind && dataRewind;
}

bool OutputPMMR::Flush()
{
	LoggerAPI::LogTrace(StringUtil::Format("OutputPMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_pHashFile->Flush();
	const bool dataFlush = m_pDataFile->Flush();
	const bool leafSetFlush = m_leafSet.Flush();
	// TODO: const bool pruneFlush = m_pruneList.Flush();

	return hashFlush && dataFlush && leafSetFlush;
}

bool OutputPMMR::Discard()
{
	LoggerAPI::LogInfo(StringUtil::Format("OutputPMMR::Discard - Discarding changes since last flush."));
	const bool hashDiscard = m_pHashFile->Discard();
	const bool dataDiscard = m_pDataFile->Discard();
	m_leafSet.Discard();

	// TODO: const bool pruneDiscard = m_pruneList.Discard();

	return hashDiscard && dataDiscard;
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