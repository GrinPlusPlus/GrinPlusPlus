#pragma once

#include "HeaderInfo.h"

#include <Core/Models/BlockHeader.h>
#include <Database/BlockDb.h>

class DifficultyCalculator
{
public:
	DifficultyCalculator(std::shared_ptr<const IBlockDB> pBlockDB);

	HeaderInfo CalculateNextDifficulty(const BlockHeader& blockHeader) const;

private:
	HeaderInfo NextDMA(const BlockHeader& header) const;
	HeaderInfo NextWTEMA(const BlockHeader& header) const;

	uint64_t ARCount(const std::vector<HeaderInfo>& difficultyData) const;
	uint64_t ScalingFactorSum(const std::vector<HeaderInfo>& difficultyData) const;
	uint32_t SecondaryPOWScaling(const uint64_t height, const std::vector<HeaderInfo>& difficultyData) const;

	std::shared_ptr<const IBlockDB> m_pBlockDB;
};