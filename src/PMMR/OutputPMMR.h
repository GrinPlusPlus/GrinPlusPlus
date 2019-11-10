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
		std::shared_ptr<LeafSet> pLeafSet = LeafSet::Load(txHashSetDirectory + "output/pmmr_leafset.bin");
		std::shared_ptr<PruneList> pPruneList = PruneList::Load(txHashSetDirectory + "output/pmmr_prun.bin");
		std::shared_ptr<DataFile<OUTPUT_SIZE>> pDataFile = DataFile<OUTPUT_SIZE>::Load(txHashSetDirectory + "output/pmmr_data.bin");

		return std::make_shared<OutputPMMR>(OutputPMMR(pHashFile, pLeafSet, pPruneList, pDataFile));
	}

	virtual ~OutputPMMR() = default;

private:
	OutputPMMR(
		std::shared_ptr<HashFile> pHashFile,
		std::shared_ptr<LeafSet> pLeafSet,
		std::shared_ptr<PruneList> pPruneList,
		std::shared_ptr<DataFile<OUTPUT_SIZE>> pDataFile)
		: PruneableMMR<OUTPUT_SIZE, OutputIdentifier>(pHashFile, pLeafSet, pPruneList, pDataFile)
	{

	}
};