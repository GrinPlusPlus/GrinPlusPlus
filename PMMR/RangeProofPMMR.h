#pragma once

#include "Common/MMR.h"
#include "Common/CRoaring/roaring.hh"
#include "Common/LeafSet.h"
#include "Common/PruneList.h"
#include "Common/HashFile.h"
#include "Common/DataFile.h"

#include <Config/Config.h>

#define RANGE_PROOF_SIZE 683

class RangeProofPMMR : public MMR
{
public:
	static RangeProofPMMR* Load(const Config& config);
	~RangeProofPMMR();

	bool Append(const RangeProof& rangeProof);
	bool Remove(const uint64_t mmrIndex);

	virtual Hash Root(const uint64_t mmrIndex) const override final;
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final;
	std::unique_ptr<RangeProof> GetRangeProofAt(const uint64_t mmrIndex) const;
	virtual uint64_t GetSize() const override final;

	virtual bool Rewind(const uint64_t size) override final;
	virtual bool Flush() override final;
	virtual bool Discard() override final;

private:
	RangeProofPMMR(const Config& config, HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>* pDataFile);

	const Config& m_config;
	HashFile* m_pHashFile;
	LeafSet m_leafSet;
	PruneList m_pruneList;
	DataFile<RANGE_PROOF_SIZE>* m_pDataFile;
};