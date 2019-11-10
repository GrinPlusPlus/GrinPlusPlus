#pragma once

#include "Common/PruneableMMR.h"

#include <Crypto/RangeProof.h>

#define RANGE_PROOF_SIZE 683

class RangeProofPMMR : public PruneableMMR<RANGE_PROOF_SIZE, RangeProof>
{
public:
	static std::shared_ptr<RangeProofPMMR> Load(const std::string& txHashSetDirectory)
	{
		std::shared_ptr<HashFile> pHashFile = HashFile::Load(txHashSetDirectory + "rangeproof/pmmr_hash.bin");
		std::shared_ptr<LeafSet> pLeafSet = LeafSet::Load(txHashSetDirectory + "rangeproof/pmmr_leafset.bin");
		std::shared_ptr<PruneList> pPruneList = PruneList::Load(txHashSetDirectory + "rangeproof/pmmr_prun.bin");
		std::shared_ptr<DataFile<RANGE_PROOF_SIZE>> pDataFile = DataFile<RANGE_PROOF_SIZE>::Load(txHashSetDirectory + "rangeproof/pmmr_data.bin");

		return std::make_shared<RangeProofPMMR>(RangeProofPMMR(pHashFile, pLeafSet, pPruneList, pDataFile));
	}

	virtual ~RangeProofPMMR() = default;

private:
	RangeProofPMMR(
		std::shared_ptr<HashFile> pHashFile,
		std::shared_ptr<LeafSet> pLeafSet,
		std::shared_ptr<PruneList> pPruneList,
		std::shared_ptr<DataFile<RANGE_PROOF_SIZE>> pDataFile)
		: PruneableMMR<RANGE_PROOF_SIZE, RangeProof>(pHashFile, pLeafSet, pPruneList, pDataFile)
	{

	}
};