#include "RangeProofPMMR.h"
#include "Common/MMRUtil.h"

#include <StringUtil.h>
#include <Infrastructure/Logger.h>

RangeProofPMMR::RangeProofPMMR(const Config& config, HashFile&& hashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>&& dataFile)
	: m_config(config),
	m_hashFile(std::move(hashFile)),
	m_leafSet(std::move(leafSet)),
	m_pruneList(std::move(pruneList)),
	m_dataFile(std::move(dataFile))
{

}

RangeProofPMMR* RangeProofPMMR::Load(const Config& config)
{
	HashFile hashFile(config.GetTxHashSetDirectory() + "rangeproof/pmmr_hash.bin");
	hashFile.Load();

	LeafSet leafSet(config.GetTxHashSetDirectory() + "rangeproof/pmmr_leaf.bin");
	leafSet.Load();

	PruneList pruneList(std::move(PruneList::Load(config.GetTxHashSetDirectory() + "rangeproof/pmmr_prun.bin")));

	DataFile<RANGE_PROOF_SIZE> dataFile(config.GetTxHashSetDirectory() + "rangeproof/pmmr_data.bin");
	dataFile.Load();

	return new RangeProofPMMR(config, std::move(hashFile), std::move(leafSet), std::move(pruneList), std::move(dataFile));
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

	return std::make_unique<Hash>(m_hashFile.GetHashAt(shiftedIndex));
}

uint64_t RangeProofPMMR::GetSize() const
{
	const uint64_t totalShift = m_pruneList.GetTotalShift();

	return totalShift + m_hashFile.GetSize();
}

bool RangeProofPMMR::Rewind(const uint64_t lastMMRIndex)
{
	const bool hashRewind = m_hashFile.Rewind(lastMMRIndex - m_pruneList.GetShift(lastMMRIndex));
	const bool dataRewind = m_dataFile.Rewind(MMRUtil::GetNumLeaves(lastMMRIndex) - m_pruneList.GetLeafShift(lastMMRIndex));
	// TODO: Rewind leafset && pruneList?

	return hashRewind && dataRewind;
}

bool RangeProofPMMR::Flush()
{
	LoggerAPI::LogInfo(StringUtil::Format("RangeProofPMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_hashFile.Flush();
	const bool dataFlush = m_dataFile.Flush();
	const bool leafSetFlush = m_leafSet.Flush();
	const bool pruneFlush = m_pruneList.Flush();

	return hashFlush && dataFlush && leafSetFlush && pruneFlush;
}