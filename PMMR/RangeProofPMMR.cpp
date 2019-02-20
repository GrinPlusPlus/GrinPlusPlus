#include "RangeProofPMMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

RangeProofPMMR::RangeProofPMMR(HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>* pDataFile)
	: m_pHashFile(pHashFile),
	m_leafSet(std::move(leafSet)),
	m_pruneList(std::move(pruneList)),
	m_pDataFile(pDataFile)
{

}

RangeProofPMMR::~RangeProofPMMR()
{
	delete m_pHashFile;
	m_pHashFile = nullptr;

	delete m_pDataFile;
	m_pDataFile = nullptr;
}

RangeProofPMMR* RangeProofPMMR::Load(const std::string& txHashSetDirectory)
{
	HashFile* pHashFile = new HashFile(txHashSetDirectory + "rangeproof/pmmr_hash.bin");
	pHashFile->Load();

	LeafSet leafSet(txHashSetDirectory + "rangeproof/pmmr_leaf.bin");
	leafSet.Load();

	PruneList pruneList(std::move(PruneList::Load(txHashSetDirectory + "rangeproof/pmmr_prun.bin")));

	DataFile<RANGE_PROOF_SIZE>* pDataFile = new DataFile<RANGE_PROOF_SIZE>(txHashSetDirectory + "rangeproof/pmmr_data.bin");
	pDataFile->Load();

	return new RangeProofPMMR(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile);
}

bool RangeProofPMMR::Append(const RangeProof& rangeProof)
{
	// Add to LeafSet
	const uint64_t totalShift = m_pruneList.GetTotalShift();
	const uint64_t mmrIndex = m_pHashFile->GetSize() + totalShift;
	m_leafSet.Add((uint32_t)mmrIndex);

	// Add to data file
	Serializer serializer;
	rangeProof.Serialize(serializer);
	m_pDataFile->AddData(serializer.GetBytes());

	// Add hashes
	MMRHashUtil::AddHashes(*m_pHashFile, serializer.GetBytes(), &m_pruneList);

	return true;
}

bool RangeProofPMMR::Remove(const uint64_t mmrIndex)
{
	LoggerAPI::LogTrace("RangeProofPMMR::Remove - Spending rangeproof at index " + std::to_string(mmrIndex));

	if (!MMRUtil::IsLeaf(mmrIndex))
	{
		LoggerAPI::LogWarning("RangeProofPMMR::Remove - RangeProof is not a leaf " + std::to_string(mmrIndex));
		return false;
	}

	if (!m_leafSet.Contains(mmrIndex))
	{
		LoggerAPI::LogWarning("RangeProofPMMR::Remove - LeafSet does not contain output " + std::to_string(mmrIndex));
		return false;
	}

	m_leafSet.Remove((uint32_t)mmrIndex);

	return true;
}

Hash RangeProofPMMR::Root(const uint64_t size) const
{
	return MMRHashUtil::Root(*m_pHashFile, size, &m_pruneList);
}

std::unique_ptr<Hash> RangeProofPMMR::GetHashAt(const uint64_t mmrIndex) const
{
	Hash hash = MMRHashUtil::GetHashAt(*m_pHashFile, mmrIndex, &m_pruneList);
	if (hash == ZERO_HASH)
	{
		return std::unique_ptr<Hash>(nullptr);
	}

	return std::make_unique<Hash>(std::move(hash));
}

std::vector<Hash> RangeProofPMMR::GetLastLeafHashes(const uint64_t numHashes) const
{
	return MMRHashUtil::GetLastLeafHashes(*m_pHashFile, &m_leafSet, &m_pruneList, numHashes);
}

std::unique_ptr<RangeProof> RangeProofPMMR::GetRangeProofAt(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		if (m_leafSet.Contains(mmrIndex))
		{
			const uint64_t shift = m_pruneList.GetLeafShift(mmrIndex);
			const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);
			const uint64_t shiftedIndex = ((numLeaves - 1) - shift);

			std::vector<unsigned char> data;
			m_pDataFile->GetDataAt(shiftedIndex, data);

			if (data.size() == RANGE_PROOF_SIZE)
			{
				ByteBuffer byteBuffer(data);
				return std::make_unique<RangeProof>(RangeProof::Deserialize(byteBuffer));
			}
		}
	}

	return std::unique_ptr<RangeProof>(nullptr);
}

uint64_t RangeProofPMMR::GetSize() const
{
	const uint64_t totalShift = m_pruneList.GetTotalShift();

	return totalShift + m_pHashFile->GetSize();
}

bool RangeProofPMMR::Rewind(const uint64_t size, const std::optional<Roaring>& leavesToAddOpt)
{
	const bool hashRewind = m_pHashFile->Rewind(size - m_pruneList.GetShift(size - 1));
	const bool dataRewind = m_pDataFile->Rewind(MMRUtil::GetNumLeaves(size - 1) - m_pruneList.GetLeafShift(size - 1));

	if (leavesToAddOpt.has_value())
	{
		m_leafSet.Rewind(size, leavesToAddOpt.value());
	}

	return hashRewind && dataRewind;
}

bool RangeProofPMMR::Flush()
{
	LoggerAPI::LogTrace(StringUtil::Format("RangeProofPMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_pHashFile->Flush();
	const bool dataFlush = m_pDataFile->Flush();
	const bool leafSetFlush = m_leafSet.Flush();

	return hashFlush && dataFlush && leafSetFlush;
}

bool RangeProofPMMR::Discard()
{
	LoggerAPI::LogInfo(StringUtil::Format("RangeProofPMMR::Discard - Discarding changes since last flush."));
	const bool hashDiscard = m_pHashFile->Discard();
	const bool dataDiscard = m_pDataFile->Discard();
	m_leafSet.Discard();

	// TODO: const bool pruneDiscard = m_pruneList.Discard();

	return hashDiscard && dataDiscard;
}