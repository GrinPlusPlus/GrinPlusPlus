#pragma once

#include <Core/Models/BlockHeader.h>
#include <Core/Config.h>
#include <Database/BlockDb.h>

class PoWValidator
{
public:
	PoWValidator(const Config& config, std::shared_ptr<const IBlockDB> pBlockDB);

	bool IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const;

private:
	uint64_t GetMaximumDifficulty(const BlockHeader& header) const;

	const Config& m_config;
	std::shared_ptr<const IBlockDB> m_pBlockDB;
};