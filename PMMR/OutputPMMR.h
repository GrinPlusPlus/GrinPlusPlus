#pragma once

#include "Common/MMR.h"
#include "Common/LeafSet.h"
#include "Common/PruneList.h"
#include "Common/HashFile.h"

#include <Core/DataFile.h>
#include <Core/Models/OutputIdentifier.h>
#include <Database/BlockDb.h>
#include <Crypto/Hash.h>
#include <Core/CRoaring/roaring.hh>

#define OUTPUT_SIZE 34

class OutputPMMR : public MMR
{
public:
	static OutputPMMR* Load(const std::string& txHashSetDirectory, IBlockDB& blockDB);
	~OutputPMMR();

	bool Append(const OutputIdentifier& output, const uint64_t blockHeight);
	bool Remove(const uint64_t mmrIndex);
	bool Rewind(const uint64_t size, const std::optional<Roaring>& leavesToAddOpt);

	virtual Hash Root(const uint64_t mmrIndex) const override final;
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final;
	virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const override final;
	virtual uint64_t GetSize() const override final;

	virtual bool Flush() override final;
	virtual bool Discard() override final;

	bool IsUnspent(const uint64_t mmrIndex) const;
	std::unique_ptr<OutputIdentifier> GetOutputAt(const uint64_t mmrIndex) const;

private:
	OutputPMMR(IBlockDB& blockDB, HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<OUTPUT_SIZE>* pDataFile);

	Roaring DetermineLeavesToRemove(const uint64_t cutoffSize, const Roaring& rewindRmPos) const;
	Roaring DetermineNodesToRemove(const Roaring& leavesToRemove) const;

	IBlockDB& m_blockDB;

	HashFile* m_pHashFile;
	LeafSet m_leafSet;
	PruneList m_pruneList;
	DataFile<OUTPUT_SIZE>* m_pDataFile;
};