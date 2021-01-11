#pragma once

#include "Common/PruneableMMR.h"

#include <Core/Global.h>
#include <Core/Models/OutputIdentifier.h>
#include <filesystem.h>

#define OUTPUT_SIZE 34

class OutputPMMR : public PruneableMMR<OUTPUT_SIZE, OutputIdentifier>
{
public:
	static std::shared_ptr<OutputPMMR> Load(const fs::path& txHashSetPath)
	{
		const auto genesisOutput = OutputIdentifier::FromOutput(Global::GetGenesisBlock().GetOutputs().front());

		std::shared_ptr<HashFile> pHashFile = HashFile::Load(txHashSetPath / "output" / "pmmr_hash.bin");

		if (!FileUtil::Exists(txHashSetPath / "output" / "pmmr_leafset.bin") && FileUtil::Exists(txHashSetPath / "output" / "pmmr_leaf.bin"))
		{
			Roaring outputBitmap;
			std::vector<unsigned char> outputBytes;
			if (FileUtil::ReadFile(txHashSetPath / "output" / "pmmr_leaf.bin", outputBytes))
			{
				outputBitmap = Roaring::readSafe((const char*)outputBytes.data(), outputBytes.size());
			}
			BitmapFile::Create(txHashSetPath / "output" / "pmmr_leafset.bin", outputBitmap);
		}

		std::shared_ptr<LeafSet> pLeafSet = LeafSet::Load(txHashSetPath / "output" / "pmmr_leafset.bin");
		std::shared_ptr<PruneList> pPruneList = PruneList::Load(txHashSetPath / "output" / "pmmr_prun.bin");
		std::shared_ptr<DataFile<OUTPUT_SIZE>> pDataFile = DataFile<OUTPUT_SIZE>::Load(txHashSetPath / "output" / "pmmr_data.bin");

		auto pOutputPMMR =  std::shared_ptr<OutputPMMR>(new OutputPMMR(pHashFile, pLeafSet, pPruneList, pDataFile));
		if (pHashFile->GetSize() == 0)
		{
			pOutputPMMR->Append(OutputIdentifier::FromOutput(Global::GetGenesisBlock().GetOutputs().front()));
		}

		return pOutputPMMR;
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