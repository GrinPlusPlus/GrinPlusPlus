#include "RangeProofPMMR.h"
#include "Common/MMRUtil.h"

#include <StringUtil.h>
#include <Infrastructure/Logger.h>

RangeProofPMMR::RangeProofPMMR(const Config& config, HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>* pDataFile)
	: m_config(config),
	m_pHashFile(pHashFile),
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

RangeProofPMMR* RangeProofPMMR::Load(const Config& config)
{
	HashFile* pHashFile = new HashFile(config.GetTxHashSetDirectory() + "rangeproof/pmmr_hash.bin");
	pHashFile->Load();

	LeafSet leafSet(config.GetTxHashSetDirectory() + "rangeproof/pmmr_leaf.bin");
	leafSet.Load();

	PruneList pruneList(std::move(PruneList::Load(config.GetTxHashSetDirectory() + "rangeproof/pmmr_prun.bin")));

	DataFile<RANGE_PROOF_SIZE>* pDataFile = new DataFile<RANGE_PROOF_SIZE>(config.GetTxHashSetDirectory() + "rangeproof/pmmr_data.bin");
	pDataFile->Load();

	return new RangeProofPMMR(config, pHashFile, std::move(leafSet), std::move(pruneList), pDataFile);
}

Hash RangeProofPMMR::Root(const uint64_t size) const
{
	LoggerAPI::LogTrace("RangeProofPMMR::Root - Calculating root of MMR with size " + std::to_string(size));

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

std::unique_ptr<Hash> RangeProofPMMR::GetHashAt(const uint64_t mmrIndex) const
{
	if (m_pruneList.IsPruned(mmrIndex) && !m_pruneList.IsPrunedRoot(mmrIndex))
	{
		return std::unique_ptr<Hash>(nullptr);
	}

	const uint64_t shift = m_pruneList.GetShift(mmrIndex);
	const uint64_t shiftedIndex = (mmrIndex - shift);

	return std::make_unique<Hash>(m_pHashFile->GetHashAt(shiftedIndex));
}

std::unique_ptr<RangeProof> RangeProofPMMR::GetRangeProofAt(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		if (m_leafSet.Contains(mmrIndex))
		{
			if (m_pruneList.IsCompacted(mmrIndex))
			{
				return std::unique_ptr<RangeProof>(nullptr);
			}

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

bool RangeProofPMMR::Rewind(const uint64_t lastMMRIndex)
{
	const bool hashRewind = m_pHashFile->Rewind(lastMMRIndex - m_pruneList.GetShift(lastMMRIndex));
	const bool dataRewind = m_pDataFile->Rewind(MMRUtil::GetNumLeaves(lastMMRIndex) - m_pruneList.GetLeafShift(lastMMRIndex));
	// TODO: Rewind leafset && pruneList?

	return hashRewind && dataRewind;
}

bool RangeProofPMMR::Flush()
{
	LoggerAPI::LogInfo(StringUtil::Format("RangeProofPMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_pHashFile->Flush();
	const bool dataFlush = m_pDataFile->Flush();
	const bool leafSetFlush = m_leafSet.Flush();
	const bool pruneFlush = m_pruneList.Flush();

	return hashFlush && dataFlush && leafSetFlush && pruneFlush;
}