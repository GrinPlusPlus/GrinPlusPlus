#pragma once

#include "HeaderInfo.h"

#include <Database/BlockDb.h>
#include <Core/Models/BlockHeader.h>
#include <vector>

class DifficultyLoader
{
public:
	DifficultyLoader(std::shared_ptr<const IBlockDB> pBlockDB);

	std::vector<HeaderInfo> LoadDifficultyData(const BlockHeader& header) const;

private:
	std::vector<HeaderInfo> PadDifficultyData(std::vector<HeaderInfo>& difficultyData) const;

	std::shared_ptr<const IBlockDB> m_pBlockDB;
};