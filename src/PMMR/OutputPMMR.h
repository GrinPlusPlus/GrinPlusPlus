#pragma once

#include "Common/PruneableMMR.h"

#include <Core/Models/OutputIdentifier.h>

#define OUTPUT_SIZE 34

class OutputPMMR : public PruneableMMR<OUTPUT_SIZE, OutputIdentifier>
{
public:
	static std::shared_ptr<OutputPMMR> Load(const std::string& txHashSetDirectory)
	{
		std::shared_ptr<HashFile> pHashFile = HashFile::Load(txHashSetDirectory + "output/pmmr_hash.bin");

		LeafSet leafSet(txHashSetDirectory + "output/pmmr_leaf.bin");
		leafSet.Load();

		PruneList pruneList(PruneList::Load(txHashSetDirectory + "output/pmmr_prun.bin"));

		std::shared_ptr<DataFile<OUTPUT_SIZE>> pDataFile = DataFile<OUTPUT_SIZE>::Load(txHashSetDirectory + "output/pmmr_data.bin");

		return std::make_shared<OutputPMMR>(OutputPMMR(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile));
	}

	virtual ~OutputPMMR() = default;

private:
	OutputPMMR(std::shared_ptr<HashFile> pHashFile, LeafSet&& leafSet, PruneList&& pruneList, std::shared_ptr<DataFile<OUTPUT_SIZE>> pDataFile)
		: PruneableMMR<OUTPUT_SIZE, OutputIdentifier>(pHashFile, std::move(leafSet), std::move(pruneList), pDataFile)
	{

	}
};