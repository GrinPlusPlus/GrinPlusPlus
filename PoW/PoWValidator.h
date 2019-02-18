#pragma once

#include <Core/Models/BlockHeader.h>
#include <Config/Config.h>
#include <Database/BlockDb.h>

class PoWValidator
{
public:
	PoWValidator(const Config& config, const IBlockDB& blockDB);

	bool IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const;

private:
	uint64_t GetMaximumDifficulty(const BlockHeader& header) const;

	const Config& m_config;
	const IBlockDB& m_blockDB;
};