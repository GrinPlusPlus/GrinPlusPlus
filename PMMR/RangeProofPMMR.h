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

	virtual Hash Root(const uint64_t mmrIndex) const override final;
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final;
	virtual uint64_t GetSize() const override final;

	virtual bool Rewind(const uint64_t lastMMRIndex) override final;
	virtual bool Flush() override final;

private:
	RangeProofPMMR(const Config& config, HashFile&& hashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<RANGE_PROOF_SIZE>&& dataFile);

	const Config& m_config;
	HashFile m_hashFile;
	LeafSet m_leafSet;
	PruneList m_pruneList;
	DataFile<RANGE_PROOF_SIZE> m_dataFile;
};