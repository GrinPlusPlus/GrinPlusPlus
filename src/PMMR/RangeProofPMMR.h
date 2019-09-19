#pragma once

#include "Common/PruneableMMR.h"

#include <Crypto/RangeProof.h>

#define RANGE_PROOF_SIZE 683

class RangeProofPMMR : public PruneableMMR<RANGE_PROOF_SIZE, RangeProof>
{
public:
	static RangeProofPMMR* Load(const std::string& txHashSetDirectory)
	{
		HashFile* pHashFile = new HashFile(txHashSetDirectory + "rangeproof/pmmr_hash.bin");
		pHashFile->Load();

		LeafSet leafSet(txHashSetDirectory + "rangeproof/pmmr_leaf.bin");
		leafSet.Load();

		PruneList pruneList(PruneList::Load(txHashSetDirectory + "rangeproof/pmmr_prun.bin"));

		DataFile<RANGE_PROOF_SIZE>* pDataFile = new DataFile<RANGE_PROOF_SIZE>(txHashSetDirectory + "rangeproof/pmmr_data.bin");
		pDataFile->Load();

		return new RangeProofPMMR(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile);
	}

	virtual ~RangeProofPMMR() = default;

private:
	RangeProofPMMR(HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>* pDataFile)
		: PruneableMMR<RANGE_PROOF_SIZE, RangeProof>(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile)
	{

	}
};