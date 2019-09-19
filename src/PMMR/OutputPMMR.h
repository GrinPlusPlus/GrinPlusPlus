#pragma once

#include "Common/PruneableMMR.h"

#include <Core/Models/OutputIdentifier.h>

#define OUTPUT_SIZE 34

class OutputPMMR : public PruneableMMR<OUTPUT_SIZE, OutputIdentifier>
{
public:
	static OutputPMMR* Load(const std::string& txHashSetDirectory)
	{
		HashFile* pHashFile = new HashFile(txHashSetDirectory + "output/pmmr_hash.bin");
		pHashFile->Load();

		LeafSet leafSet(txHashSetDirectory + "output/pmmr_leaf.bin");
		leafSet.Load();

		PruneList pruneList(PruneList::Load(txHashSetDirectory + "output/pmmr_prun.bin"));

		DataFile<OUTPUT_SIZE>* pDataFile = new DataFile<OUTPUT_SIZE>(txHashSetDirectory + "output/pmmr_data.bin");
		pDataFile->Load();

		return new OutputPMMR(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile);
	}

	virtual ~OutputPMMR() = default;

private:
	OutputPMMR(HashFile* pHashFile, LeafSet&& leafSet, PruneList&& pruneList, DataFile<OUTPUT_SIZE>* pDataFile)
		: PruneableMMR<OUTPUT_SIZE, OutputIdentifier>(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile)
	{

	}
};