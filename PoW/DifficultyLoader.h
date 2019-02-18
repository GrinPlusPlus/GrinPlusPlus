#pragma once

#include "HeaderInfo.h"

#include <Database/BlockDb.h>
#include <Core/Models/BlockHeader.h>
#include <vector>

class DifficultyLoader
{
public:
	DifficultyLoader(const IBlockDB& blockDB);

	std::vector<HeaderInfo> LoadDifficultyData(const BlockHeader& header) const;

private:
	std::unique_ptr<BlockHeader> LoadHeader(const Hash& headerHash) const;
	std::vector<HeaderInfo> PadDifficultyData(std::vector<HeaderInfo>& difficultyData) const;

	const IBlockDB& m_blockDB;
};