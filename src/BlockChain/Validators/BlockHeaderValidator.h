#pragma once

#include <Core/Models/BlockHeader.h>
#include <Core/Config.h>

// Forward Declarations
class IHeaderMMR;
class IBlockDB;

class BlockHeaderValidator
{
public:
	BlockHeaderValidator(const Config& config, std::shared_ptr<const IBlockDB> pBlockDB, std::shared_ptr<const IHeaderMMR> pHeaderMMR);

	bool IsValidHeader(const BlockHeader& header, const BlockHeader& previousHeader) const;

	const Config& m_config;
	std::shared_ptr<const IBlockDB> m_pBlockDB;
	std::shared_ptr<const IHeaderMMR> m_pHeaderMMR;
};