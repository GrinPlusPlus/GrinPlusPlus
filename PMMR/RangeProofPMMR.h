#pragma once

#include "Common/MMR.h"
#include "Common/LeafSet.h"
#include "Common/PruneList.h"
#include "Common/HashFile.h"

#include <optional>
#include <Crypto/Hash.h>
#include <Core/DataFile.h>
#include <Crypto/RangeProof.h>
#include <Core/CRoaring/roaring.hh>

#define RANGE_PROOF_SIZE 683

class RangeProofPMMR : public MMR
{
public:
	static RangeProofPMMR* Load(const std::string& txHashSetDirectory);
	~RangeProofPMMR();

	bool Append(const RangeProof& rangeProof);
	bool Remove(const uint64_t mmrIndex);
	bool Rewind(const uint64_t size, const std::optional<Roaring>& leavesToAddOpt);

	virtual Hash Root(const uint64_t mmrIndex) const override final;
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final;
	virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const override final;
	std::unique_ptr<RangeProof> GetRangeProofAt(const uint64_t mmrIndex) const;
	virtual uint64_t GetSize() const override final;

	virtual bool Flush() override final;
	virtual bool Discard() override final;

private:
	RangeProofPMMR(HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>* pDataFile);

	HashFile* m_pHashFile;
	LeafSet m_leafSet;
	PruneList m_pruneList;
	DataFile<RANGE_PROOF_SIZE>* m_pDataFile;
};